#include "render_graph_vk.hh"

#include "graphite/imgui.hh"
#include "graphite/vram_bank.hh"
#include "graphite/nodes/node.hh"
#include "graphite/nodes/compute_node.hh"
#include "wrapper/translate_vk.hh"

Result<void> RenderGraph::init(GPUAdapter& gpu) {
    this->gpu = &gpu;

    /* Allocate graph executions ring buffer */
    graphs = new GraphExecution[max_graphs_in_flight] {};
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

    /* Allocate graph execution resources */
    for (u32 i = 0u; i < max_graphs_in_flight; ++i) {
        if (vkAllocateCommandBuffers(gpu.logical_device, &cmd_ai, &graphs[i].cmd) != VK_SUCCESS)
            return Err("failed to allocate command buffer for graph.");
        if (vkCreateFence(gpu.logical_device, &fence_ci, nullptr, &graphs[i].flight_fence) != VK_SUCCESS)
            return Err("failed to create in-flight fence for graph.");
        if (vkCreateSemaphore(gpu.logical_device, &sema_ci, nullptr, &graphs[i].start_semaphore) != VK_SUCCESS)
            return Err("failed to create start semaphore for graph.");
    }

    /* Initialize the pipeline cache */
    pipeline_cache.init(gpu);

    return Ok();
}

Result<void> RenderGraph::dispatch() {
    /* Get the next graph in the graph executions ring buffer */
    const GraphExecution& graph = active_graph();

    /* In nanoseconds (one second) */
    constexpr uint64_t TIMEOUT = 1'000'000'000u;

    /* Wait for this graph to be out of flight before re-using its resources */
    if (vkWaitForFences(gpu->logical_device, 1u, &graph.flight_fence, true, TIMEOUT) != VK_SUCCESS) {
        return Err("failed while waiting for graph in-flight fence.");
    }
    
    /* If this graph has a render target, fetch it */
    const bool has_target = target.is_null() == false;
    RenderTargetSlot* rt = nullptr;
    if (has_target) {
        /* Acquire the next render target image from the swapchain */
        rt = &gpu->get_vram_bank().get_render_target(target);
        const VkResult swapchain_result = vkAcquireNextImageKHR(gpu->logical_device, rt->swapchain, TIMEOUT, graph.start_semaphore, VK_NULL_HANDLE, &rt->current_image);

        /* Don't render anything if the swapchain is out of date */
        if (swapchain_result == VK_ERROR_OUT_OF_DATE_KHR) return Ok();

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
        VkImageMemoryBarrier rt_barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        rt_barrier.oldLayout = rt->old_layout();
        rt_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        rt->old_layout() = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        rt_barrier.image = rt->image();
        rt_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u };
        vkCmdPipelineBarrier(graph.cmd, 
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0u, 0u, nullptr, 0u, nullptr, 1u, &rt_barrier
        );
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

    return Ok();
}

Result<void> RenderGraph::queue_wave(const GraphExecution& graph, u32 start, u32 end) {
    /* Insert sync barriers for wave descriptors */
    const Result sync_result = wave_sync_descriptors(*this, start, end);
    if (sync_result.is_err()) return sync_result;

    /* Queue each node in the wave */
    for (u32 i = start; i < end; ++i) {
        const Node& node = *nodes[waves[i].lane];

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

    /* Calculate the dispatch size */
    const u32 dispatch_x = div_up(node.work_x, node.group_x);
    const u32 dispatch_y = div_up(node.work_y, node.group_y);
    const u32 dispatch_z = div_up(node.work_z, node.group_z);

    /* Dispatch the compute pipeline */
    vkCmdDispatch(graph.cmd, dispatch_x, dispatch_y, dispatch_z);

    return Ok();
}

Result<void> RenderGraph::queue_raster_node(const GraphExecution& graph, const RasterNode& node) { 
    /* Try to get the pipeline for this compute node */
    const Result cache_result = pipeline_cache.get_pipeline(shader_path, node);
    if (cache_result.is_err()) return Err(cache_result.unwrap_err());
    const Pipeline pipeline = cache_result.unwrap();

    const Result push_result = node_push_descriptors(*this, pipeline, node);
    if (push_result.is_err()) return push_result;

    /* Find all attachment resource dependencies to put in the rendering info. */
    std::vector<VkRenderingAttachmentInfo> color_attachments {};
    for (const Dependency& dep : node.dependencies) {
        /* Find attachment dependencies */
        if (has_flag(dep.flags, DependencyFlags::Attachment) == false) continue;

        /* Handle render target attachments */
        VkImageView attachment_view = VK_NULL_HANDLE;
        if (dep.resource.get_type() == ResourceType::RenderTarget) {
            attachment_view = gpu->get_vram_bank().get_render_target(target).view();
        } else {
            const ImageSlot& image = gpu->get_vram_bank().get_image(dep.resource);
            const TextureSlot& texture = gpu->get_vram_bank().get_texture(image.texture);
            if (has_flag(texture.usage, TextureUsage::ColorAttachment) == false) continue;
            attachment_view = image.view;
        }

        VkRenderingAttachmentInfo attachment { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        attachment.imageView = attachment_view;
        attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachments.emplace_back(attachment);
    }

    /* Get the render area */
    VkRect2D render_area {};
    render_area.offset = {(i32)node.raster_x, (i32)node.raster_y};
    render_area.extent = {node.raster_w, node.raster_h};
    if (render_area.extent.width == 0u || render_area.extent.height == 0u) {
        RenderTargetSlot& rt_slot = gpu->get_vram_bank().get_render_target(target);
        render_area.extent = rt_slot.extent;
    }

    /* Rendering info */
    VkRenderingInfo rendering { VK_STRUCTURE_TYPE_RENDERING_INFO };
    rendering.renderArea = render_area;
    rendering.layerCount = 1u;
    rendering.colorAttachmentCount = (u32)color_attachments.size();
    rendering.pColorAttachments = color_attachments.data();

    /* Begin rendering */
    vkCmdBeginRenderingKHR(graph.cmd, &rendering);

    /* Bind the pipeline */
    vkCmdBindPipeline(graph.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
    // TODO: Add Bindless
    /*vkCmdBindDescriptorSets(
        graph.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 1u, 1u, &vrm->bindless_set, 0u, nullptr
    );*/

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
        const BufferSlot& vertex_buffer = gpu->get_vram_bank().get_buffer(draw_call.vertex_buffer);
        const VkDeviceSize offset = 0u;
        vkCmdBindVertexBuffers(graph.cmd, 0u, 1u, &vertex_buffer.buffer, &offset);

        /* Execute the draw */
        vkCmdDraw(
            graph.cmd, draw_call.vertex_count, draw_call.instance_count, draw_call.vertex_offset, draw_call.instance_offset
        );
    }

    /* End rendering */
    vkCmdEndRenderingKHR(graph.cmd);

    return Ok();
}

void RenderGraph::queue_imgui(const GraphExecution &graph) {
    /* If this graph doesn't have a render target, don't render imgui */
    if (imgui == nullptr || target.is_null()) return;
    RenderTargetSlot& rt = gpu->get_vram_bank().get_render_target(target);

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

    /* Define the render target attachment */
    VkRenderingAttachmentInfoKHR attachment_info { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
    attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
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
}

Result<void> RenderGraph::destroy() {
    /* Wait for the queue to idle */
    vkQueueWaitIdle(gpu->queues.queue_combined);

    /* Evict the pipeline cache */
    pipeline_cache.evict();

    /* Destroy graph execution resources */
    for (u32 i = 0u; i < max_graphs_in_flight; ++i) {
        vkDestroyFence(gpu->logical_device, graphs[i].flight_fence, nullptr);
        vkDestroySemaphore(gpu->logical_device, graphs[i].start_semaphore, nullptr);
    }
    delete[] graphs;

    return Ok();
}
