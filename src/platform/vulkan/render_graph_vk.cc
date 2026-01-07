#include "render_graph_vk.hh"

#include <utility>

#include "graphite/imgui.hh"
#include "graphite/vram_bank.hh"
#include "graphite/nodes/node.hh"
#include "graphite/nodes/compute_node.hh"
#include "wrapper/translate_vk.hh"

Result<void> RenderGraph::init(GPUAdapter& gpu) {
    this->gpu = &gpu;

    /* Allocate graph executions ring buffer */
    graphs = new GraphExecution[max_graphs_in_flight] {};
    resources = new std::vector<BindHandle>[max_graphs_in_flight] {};
    active_graph_index = 0u;

    /* Command buffer allocation info */
    VkCommandBufferAllocateInfo cmd_ai { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmd_ai.commandPool = gpu.cmd_pool;
    cmd_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_ai.commandBufferCount = 1u;

    /* Fence creation info */
    VkFenceCreateInfo fence_ci { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    /* Semaphore creation info */
    const VkSemaphoreCreateInfo sema_ci { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    /* Graph staging buffer creation info */
    VkBufferCreateInfo staging_buffer_ci { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    staging_buffer_ci.size = graph_staging_limit;
    staging_buffer_ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    staging_buffer_ci.queueFamilyIndexCount = 1u;
    staging_buffer_ci.pQueueFamilyIndices = &gpu.queue_families.queue_combined;

    /* Graph staging memory allocation info */
    VmaAllocationCreateInfo alloc_ci {};
    alloc_ci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;

    /* Allocate graph execution resources */
    for (u32 i = 0u; i < max_graphs_in_flight; ++i) {
        if (vkAllocateCommandBuffers(gpu.logical_device, &cmd_ai, &graphs[i].cmd) != VK_SUCCESS)
            return Err("failed to allocate command buffer for graph.");
        std::string cb_name = "Render Graph #" + std::to_string(i) + " Command Buffer";
        gpu.set_object_name(VkObjectType::VK_OBJECT_TYPE_COMMAND_BUFFER, (u64)graphs[i].cmd, cb_name.c_str());

        if (vkCreateFence(gpu.logical_device, &fence_ci, nullptr, &graphs[i].flight_fence) != VK_SUCCESS)
            return Err("failed to create in-flight fence for graph.");
        std::string fence_name = "Render Graph #" + std::to_string(i) + " In-Flight Fence";
        gpu.set_object_name(VkObjectType::VK_OBJECT_TYPE_FENCE, (u64)graphs[i].flight_fence, fence_name.c_str());

        if (vkCreateSemaphore(gpu.logical_device, &sema_ci, nullptr, &graphs[i].start_semaphore) != VK_SUCCESS)
            return Err("failed to create start semaphore for graph.");
        std::string sema_name = "Render Graph # " + std::to_string(i) + " Start Semaphore";
        gpu.set_object_name(VkObjectType::VK_OBJECT_TYPE_FENCE, (u64)graphs[i].flight_fence, sema_name.c_str());

        /* Create the graph staging buffer & allocate it using VMA */
        if (vmaCreateBuffer(gpu.get_vram_bank().vma_allocator, &staging_buffer_ci, &alloc_ci, &graphs[i].staging_buffer, &graphs[i].staging_alloc, nullptr) != VK_SUCCESS) { 
            return Err("failed to create staging buffer for graph.");
        }
        std::string sb_name = "Render Graph # " + std::to_string(i) + " Staging Buffer";
        gpu.set_object_name(VkObjectType::VK_OBJECT_TYPE_FENCE, (u64)graphs[i].staging_buffer, sb_name.c_str());
    }

    /* Initialize the pipeline cache */
    pipeline_cache.init(gpu);

    return Ok();
}

void RenderGraph::upload_buffer(Buffer& buffer, const void* data, u64 dst_offset, u64 size) {
    if (size == 0u) return; /* Return early if there's no data to upload */

    /* Fetch the buffer resource slot from the vram bank */
    VRAMBank& bank = gpu->get_vram_bank();
    const BufferSlot& slot = bank.buffers.get(buffer);

    /* Make sure this buffer supports being a transfer destination */
    if (has_flag(slot.usage, BufferUsage::TransferDst) == false) {
        gpu->log(DebugSeverity::Warning, "attempted to upload to buffer without TransferDst flag.");
        return;
    }

    /* Get the next graph in the graph executions ring buffer */
    GraphExecution& graph = active_graph();

    /* Make sure there's enough space in the staging buffer */
    if ((graph.staging_stack_ptr + size) >= graph_staging_limit) {
        gpu->log(DebugSeverity::Warning, "ran out of staging space in graph execution.");
        return;
    }

    /* Copy data into the staging buffer */
    void* staging_memory = nullptr;
    vmaMapMemory(bank.vma_allocator, graph.staging_alloc, &staging_memory);
    const u64 src_offset = graph.staging_stack_ptr;
    u8* staging_dst = reinterpret_cast<u8*>(staging_memory) + src_offset;
    memcpy(staging_dst, data, size); 
    graph.staging_stack_ptr += size;
    vmaUnmapMemory(bank.vma_allocator, graph.staging_alloc);

    /* Staging buffer copy command */
    StagingCommand& cmd = graph.staging_commands.emplace_back();
    cmd.dst_offset = dst_offset;
    cmd.src_offset = src_offset;
    cmd.bytes = size;
    cmd.dst_resource = buffer;
}

Result<void> RenderGraph::dispatch() {
    /* Get the next graph in the graph executions ring buffer */
    const GraphExecution& graph = active_graph();

    /* In nanoseconds (one second) */
    constexpr uint64_t TIMEOUT = 1'000'000'000u;
    
    /* If this graph has a render target, fetch it */
    const bool has_target = target.is_null() == false;
    RenderTargetSlot* rt = nullptr;
    if (has_target) {
        /* Acquire the next render target image from the swapchain */
        rt = &gpu->get_vram_bank().render_targets.get(target);
        const VkResult swapchain_result = vkAcquireNextImageKHR(gpu->logical_device, rt->swapchain, TIMEOUT, graph.start_semaphore, VK_NULL_HANDLE, &rt->current_image);

        /* Don't render anything if the swapchain is out of date */
        if (swapchain_result == VK_ERROR_OUT_OF_DATE_KHR) {
            /* Reset the staging buffer for the next graph */
            GraphExecution& next_graph = active_graph();
            next_graph.staging_stack_ptr = 0u;
            next_graph.staging_commands.clear();

            return Ok();
        }

        if (swapchain_result != VK_SUCCESS) {
            return Err("failed to acquire next swapchain image for render target.");
        }
    }

    /* Begin recording commands to the graphs command buffer */
    VkCommandBufferBeginInfo cmd_begin { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmd_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(graph.cmd, &cmd_begin) != VK_SUCCESS) {
        return Err("failed to begin recording command buffer for graph.");
    }

    /* Queue staging copy commands */
    queue_staging(graph);

    /* Process all waves in the render graph */
    for (u32 s = 0u, w = 0u, e = 0u; e <= waves.size(); ++e) {
        /* If this lane is the start of a new wave */
        if (e == waves.size() || waves[e].wave != w) {
            w++;
            const Result wave_result = queue_wave(graph, s, e);
            if (wave_result.is_err()) return Err(wave_result.unwrap_err());
            s = e;
        }
    }

    /* Queue immediate mode GUI render commands */
    queue_imgui(graph);

    /* Insert render target pipeline barrier at the end of the command buffer */
    if (has_target) {
        /* Imgui image sync barrier */
        VkImageMemoryBarrier2 rt_barrier {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        rt_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        rt_barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        rt_barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        rt_barrier.dstAccessMask = VK_ACCESS_2_NONE;
        rt_barrier.oldLayout = rt->old_layout();
        rt_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        rt->old_layout() = rt_barrier.newLayout;
        rt_barrier.image = rt->image();
        rt_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u };

        /* Render target dependency info */
        VkDependencyInfo rt_dep_info {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        rt_dep_info.imageMemoryBarrierCount = 1u;
        rt_dep_info.pImageMemoryBarriers = &rt_barrier;

        vkCmdPipelineBarrier2KHR(graph.cmd, &rt_dep_info);

        // VkImageMemoryBarrier rt_barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        // rt_barrier.oldLayout = rt->old_layout();
        // rt_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // rt->old_layout() = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // rt_barrier.image = rt->image();
        // rt_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u };
        // vkCmdPipelineBarrier(graph.cmd, 
        //     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        //     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        //     0u, 0u, nullptr, 0u, nullptr, 1u, &rt_barrier
        // );
    }

    /* Finish recording commands to the graphs command buffer */
    vkEndCommandBuffer(graph.cmd);

    /* Reset the in-flight fence */
    if (vkResetFences(gpu->logical_device, 1u, &graph.flight_fence) != VK_SUCCESS) {
        return Err("failed to reset graph in-flight fence.");
    }

    /* Graph submission info */
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo submit { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1u;
    submit.pCommandBuffers = &graph.cmd;
    if (has_target) {
        submit.signalSemaphoreCount = 1u; /* Signal when the work completes */
        submit.pSignalSemaphores = &rt->semaphore();
        submit.waitSemaphoreCount = 1u; /* Wait for image acquired */
        submit.pWaitSemaphores = &graph.start_semaphore;
    }

    /* Submit the graph commands to the queue */
    if (vkQueueSubmit(gpu->queues.queue_combined, 1u, &submit, graph.flight_fence) != VK_SUCCESS) {
        return Err("failed to submit graph commands.");
    }

    /* Present the graph results after rendering completes */
    if (has_target) {
        /* Presentation info */
        VkPresentInfoKHR present { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present.waitSemaphoreCount = 1u; /* Wait for render to complete */
        present.pWaitSemaphores = &rt->semaphore();
        present.swapchainCount = 1u;
        present.pSwapchains = &rt->swapchain;
        present.pImageIndices = &rt->current_image;

        /* Queue presentation (ignore the out of date error) */
        const VkResult present_result = vkQueuePresentKHR(gpu->queues.queue_combined, &present);
        if (present_result != VK_SUCCESS && present_result != VK_ERROR_OUT_OF_DATE_KHR) {
            return Err("failed to present to render target.");
        }
    }

    /* Update the active graph index */
    next_graph();

    /* Reset the staging buffer for the next graph */
    GraphExecution& next_graph = active_graph();
    next_graph.staging_stack_ptr = 0u;
    next_graph.staging_commands.clear();

    return Ok();
}

Result<void> RenderGraph::wait_until_safe() {
    /* In nanoseconds (one second) */
    constexpr uint64_t TIMEOUT = 1'000'000'000u;

    /* Wait for this graph to be out of flight before re-using its resources */
    if (vkWaitForFences(gpu->logical_device, 1u, &active_graph().flight_fence, true, TIMEOUT) != VK_SUCCESS) {
        return Err("failed while waiting for graph in-flight fence.");
    }
    return Ok();
}

Result<void> RenderGraph::queue_wave(const GraphExecution& graph, u32 start, u32 end) {
    /* Insert sync barriers for wave descriptors */
    const Result sync_result = wave_sync_descriptors(*this, start, end);
    if (sync_result.is_err()) return sync_result;

    /* Queue each node in the wave */
    for (u32 i = start; i < end; ++i) {
        const Node& node = *nodes[waves[i].lane];

        if (gpu->validation) {
            /* Start debug label for this node */
            VkDebugUtilsLabelEXT debug_label { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
            debug_label.pLabelName = node.label.data();
            vkCmdBeginDebugUtilsLabelEXT(graph.cmd, &debug_label);
        }

        switch (node.type) {
            case NodeType::Compute: {
                const Result node_result = queue_compute_node(graph, (const ComputeNode&)node);
                if (node_result.is_err()) return node_result;
                break;
            } 
            case NodeType::Raster: {
                const Result node_result = queue_raster_node(graph, (const RasterNode&)node);
                if (node_result.is_err()) return node_result;
                break;
            }
            default:
                return Err("unknown node type in render graph.");
        }

        /* End debug label for this node */
        if (gpu->validation) vkCmdEndDebugUtilsLabelEXT(graph.cmd);
    }

    return Ok();
}

Result<void> RenderGraph::queue_compute_node(const GraphExecution& graph, const ComputeNode& node) {
    /* Try to get the pipeline for this compute node */
    const Result cache_result = pipeline_cache.get_pipeline(shader_path, node);
    if (cache_result.is_err()) return Err(cache_result.unwrap_err());
    const Pipeline pipeline = cache_result.unwrap();

    /* Bind the compute pipeline */
    vkCmdBindPipeline(graph.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
    const Result push_result = node_push_descriptors(*this, pipeline, node);
    if (push_result.is_err()) return push_result;
    vkCmdBindDescriptorSets(graph.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 1u, 1u, &gpu->get_vram_bank().bindless_set, 0u, nullptr);

    /* Indirect Dispatch */
    if (!node.indirect_buffer.is_null())
    {
        VRAMBank& bank = gpu->get_vram_bank();
        vkCmdDispatchIndirect(graph.cmd, bank.buffers.get(node.indirect_buffer).buffer, node.indirect_offset);
        return Ok();
    }

    /* Calculate the dispatch size */
    const u32 dispatch_x = div_up(node.work_x, node.group_x);
    const u32 dispatch_y = div_up(node.work_y, node.group_y);
    const u32 dispatch_z = div_up(node.work_z, node.group_z);

    /* Dispatch the compute pipeline */
    vkCmdDispatch(graph.cmd, dispatch_x, dispatch_y, dispatch_z);

    return Ok();
}

Result<void> RenderGraph::queue_raster_node(const GraphExecution& graph, const RasterNode& node) { 
    /* Try to get the pipeline for this raster node */
    const Result cache_result = pipeline_cache.get_pipeline(shader_path, node);
    if (cache_result.is_err()) return Err(cache_result.unwrap_err());
    const Pipeline pipeline = cache_result.unwrap();

    /* Bind the pipeline */
    vkCmdBindPipeline(graph.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
  
    /* Create and submit push descriptors for this node */
    const Result push_result = node_push_descriptors(*this, pipeline, node);
    if (push_result.is_err()) return push_result;
    vkCmdBindDescriptorSets(
        graph.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 1u, 1u, &gpu->get_vram_bank().bindless_set, 0u, nullptr
    );
    VRAMBank& bank = gpu->get_vram_bank();

    /* Find all attachment resource dependencies to put in the rendering info. */
    std::vector<VkRenderingAttachmentInfo> color_attachments {};
    i32 min_raster_w = INT32_MAX, min_raster_h = INT32_MAX;
    for (const Dependency& dep : node.dependencies) {
        /* Find attachment dependencies */
        if (dep.usage != DependencyUsage::ColorAttachment) continue;

        /* Handle render target attachments */
        VkImageView attachment_view = VK_NULL_HANDLE;
        if (dep.resource.get_type() == ResourceType::RenderTarget) {
            RenderTargetSlot& rt = bank.render_targets.get(target);
            attachment_view = rt.view();
            min_raster_w = std::min(min_raster_w, (i32)rt.extent.width);
            min_raster_h = std::min(min_raster_h, (i32)rt.extent.height);
        } else {
            const ImageSlot& image = bank.images.get(dep.resource);
            const TextureSlot& texture = bank.textures.get(image.texture);
            if (has_flag(texture.usage, TextureUsage::ColorAttachment) == false) continue;
            attachment_view = image.view;
            min_raster_w = std::min(min_raster_w, (i32)texture.size.x);
            min_raster_h = std::min(min_raster_h, (i32)texture.size.y);
        }

        /* Fill in the attachment info */
        VkRenderingAttachmentInfo attachment { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        attachment.imageView = attachment_view;
        attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.loadOp = translate::load_operation(node.pixel_load_op);
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachments.emplace_back(attachment);
    }

    /* Get the render area */
    VkRect2D render_area {};
    render_area.offset = {(i32)node.raster_x, (i32)node.raster_y};
    render_area.extent = {node.raster_w, node.raster_h};
    if (min_raster_w < render_area.extent.width || min_raster_h < render_area.extent.height) {
        return Err("attempted to rasterize into attachment smaller than raster extent.");
    }

    /* Rendering info */
    VkRenderingInfo rendering { VK_STRUCTURE_TYPE_RENDERING_INFO };
    rendering.renderArea = render_area;
    rendering.layerCount = 1u;
    rendering.colorAttachmentCount = (u32)color_attachments.size();
    rendering.pColorAttachments = color_attachments.data();

    /* Begin rendering */
    vkCmdBeginRenderingKHR(graph.cmd, &rendering);

    VkViewport viewport {};
    viewport.x = (f32)render_area.offset.x;
    viewport.y = (f32)render_area.offset.y;
    viewport.width = (f32)render_area.extent.width;
    viewport.height = (f32)render_area.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    const VkRect2D scissor = render_area;

    /* Set the viewport and scissor */
    vkCmdSetViewport(graph.cmd, 0u, 1u, &viewport);
    vkCmdSetScissor(graph.cmd, 0u, 1u, &scissor);

    /* Loop over all draw calls, bind vertex buffers, draw */
    for (const DrawCall& draw_call : node.draws) {
        /* Bind the vertex buffer for this draw call */
        const BufferSlot& vertex_buffer = bank.buffers.get(draw_call.vertex_buffer);
        const VkDeviceSize offset = 0u;
        vkCmdBindVertexBuffers(graph.cmd, 0u, 1u, &vertex_buffer.buffer, &offset);

        /* Check for indirect draw */
        if (!draw_call.indirect_buffer.is_null()) {
            vkCmdDrawIndirect(
                graph.cmd, bank.buffers.get(draw_call.indirect_buffer).buffer, 0, 1, sizeof(VkDrawIndirectCommand)
            );
        } else {
            /* Execute direct draw */
            vkCmdDraw(
                graph.cmd, draw_call.vertex_count, draw_call.instance_count, draw_call.vertex_offset, draw_call.instance_offset
            );
        }
    }

    /* End rendering */
    vkCmdEndRenderingKHR(graph.cmd);

    return Ok();
}

void RenderGraph::queue_staging(const GraphExecution& graph) {
    /* Do nothing if there are no staging copies queued */
    if (graph.staging_commands.empty()) return;

    /* Compile the staging copy commands */
    std::vector<VkBufferCopy2> regions {};
    std::vector<VkCopyBufferInfo2> copies {};
    regions.reserve(graph.staging_commands.size());
    copies.reserve(graph.staging_commands.size());

    VRAMBank& bank = gpu->get_vram_bank();
    for (const StagingCommand& cmd : graph.staging_commands) {
        if (cmd.dst_resource.get_type() != ResourceType::Buffer) {
            gpu->log(DebugSeverity::Warning, "tried to stage resource which is not a buffer!");
            continue; /* TODO: Implement staging for other resources. */
        }

        /* Fill in the buffer copy region */
        VkBufferCopy2& region = regions.emplace_back();
        region.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
        region.dstOffset = cmd.dst_offset;
        region.srcOffset = cmd.src_offset;
        region.size = cmd.bytes;

        /* Fill in the buffer copy command */
        VkCopyBufferInfo2& copy = copies.emplace_back();
        copy.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
        copy.srcBuffer = graph.staging_buffer;
        copy.dstBuffer = bank.buffers.get(cmd.dst_resource).buffer;
        copy.pRegions = &region;
        copy.regionCount = 1u;
    }

    if (gpu->validation) {
        /* Start debug label for staging */
        VkDebugUtilsLabelEXT debug_label { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
        debug_label.pLabelName = "staging";
        vkCmdBeginDebugUtilsLabelEXT(graph.cmd, &debug_label);
    }

    /* Queue copy commands */
    for (const VkCopyBufferInfo2& copy : copies) {
        vkCmdCopyBuffer2KHR(graph.cmd, &copy);
    }

    /* End debug label for this node */
    if (gpu->validation) vkCmdEndDebugUtilsLabelEXT(graph.cmd);
}

void RenderGraph::queue_imgui(const GraphExecution &graph) {
#ifdef GRAPHITE_IMGUI
    /* If this graph doesn't have a render target, don't render imgui */
    if (imgui == nullptr || target.is_null()) return;
    RenderTargetSlot& rt = gpu->get_vram_bank().render_targets.get(target);

    if (gpu->validation) {
        /* Start debug label for imgui */
        VkDebugUtilsLabelEXT debug_label { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
        debug_label.pLabelName = "imgui";
        vkCmdBeginDebugUtilsLabelEXT(graph.cmd, &debug_label);
    }

    /* Transition all imgui images */
    for (const auto& [raw, imgui_image] : imgui->image_map) {
        VRAMBank& bank = gpu->get_vram_bank();

        const ImageSlot& image = bank.images.get((Image&)raw);
        TextureSlot& texture = bank.textures.get(image.texture);

        VkPipelineStageFlags2 src_stage {};
        VkAccessFlags2 src_access {};
        
        // Determine source based on current layout
        switch (texture.layout) {
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                src_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                src_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_GENERAL:
                src_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
                src_access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                src_stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
                src_access = VK_ACCESS_2_SHADER_READ_BIT;
                break;
            default:
                src_stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                src_access = VK_ACCESS_2_MEMORY_WRITE_BIT;
                break;
        }

        /* Imgui image sync barrier */
        VkImageMemoryBarrier2 barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
        barrier.srcStageMask = src_stage;
        barrier.srcAccessMask = src_access;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        barrier.oldLayout = texture.layout;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        texture.layout = barrier.newLayout;
        barrier.image = texture.image;
        barrier.subresourceRange = image.sub_range;

        /* Render target dependency info */
        VkDependencyInfo viewport_dep_info {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        viewport_dep_info.imageMemoryBarrierCount = 1u;
        viewport_dep_info.pImageMemoryBarriers = &barrier;

        /* Insert a pipeline barrier for the image */
        vkCmdPipelineBarrier2KHR(graph.cmd, &viewport_dep_info);
    }

    /* Make imgui render target sync barrier */
    VkImageMemoryBarrier2 viewport_barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    viewport_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    viewport_barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    viewport_barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    viewport_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    viewport_barrier.oldLayout = rt.old_layout();
    viewport_barrier.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
    rt.old_layout() = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
    viewport_barrier.image = rt.image();
    viewport_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};

    /* Render target dependency info */
    VkDependencyInfo viewport_dep_info { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    viewport_dep_info.imageMemoryBarrierCount = 1u;
    viewport_dep_info.pImageMemoryBarriers = &viewport_barrier;

    /* Insert a pipeline barrier before rendering the overlay */
    vkCmdPipelineBarrier2KHR(graph.cmd, &viewport_dep_info);

    /* Transition render target to color attachment for imgui rendering */
    VkImageMemoryBarrier2 rt_to_attachment { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    rt_to_attachment.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    rt_to_attachment.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    rt_to_attachment.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    rt_to_attachment.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    rt_to_attachment.oldLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
    rt_to_attachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    rt_to_attachment.image = rt.image();
    rt_to_attachment.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    rt.old_layout() = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkDependencyInfo rt_dep_info { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    rt_dep_info.imageMemoryBarrierCount = 1;
    rt_dep_info.pImageMemoryBarriers = &rt_to_attachment;
    vkCmdPipelineBarrier2KHR(graph.cmd, &rt_dep_info);

    /* Define the render target attachment */
    VkRenderingAttachmentInfoKHR attachment_info { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
    attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_info.loadOp = imgui->clear_screen ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;;
    attachment_info.imageView = rt.view();

    /* Rendering info */
    VkRenderingInfoKHR rendering_info { VK_STRUCTURE_TYPE_RENDERING_INFO_KHR };
    rendering_info.renderArea.extent = rt.extent;
    rendering_info.renderArea.offset = VkOffset2D {0, 0};
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &attachment_info;
    rendering_info.layerCount = 1;

    /* Begin dynamic rendering */
    vkCmdBeginRenderingKHR(graph.cmd, &rendering_info);

    imgui->render(graph.cmd);

    /* End dynamic rendering */
    vkCmdEndRenderingKHR(graph.cmd);

    /* End debug label for this node */
    if (gpu->validation) vkCmdEndDebugUtilsLabelEXT(graph.cmd);
#endif
}

Result<void> RenderGraph::deinit() {
    /* Wait for all graph executions to finish */
    flush_graph();

    /* Evict the pipeline cache */
    pipeline_cache.evict();

    /* Destroy graph execution resources */
    for (u32 i = 0u; i < max_graphs_in_flight; ++i) {
        vkDestroyFence(gpu->logical_device, graphs[i].flight_fence, nullptr);
        vkDestroySemaphore(gpu->logical_device, graphs[i].start_semaphore, nullptr);
        vmaDestroyBuffer(gpu->get_vram_bank().vma_allocator, graphs[i].staging_buffer, graphs[i].staging_alloc);
    }
    delete[] resources;
    delete[] graphs;

    return Ok();
}
