#include "render_graph_vk.hh"

#include "graphite/vram_bank.hh"
#include "graphite/nodes/node.hh"
#include "graphite/nodes/compute_node.hh"
#include "wrapper/translate_vk.hh"

Result<void> RenderGraph::init(GPUAdapter& gpu) {
    this->gpu = &gpu;

    /* Command pool creation info */
    VkCommandPoolCreateInfo pool_ci { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_ci.queueFamilyIndex = gpu.queue_families.queue_combined;

    /* Create command pool for command buffers */
    if (vkCreateCommandPool(gpu.logical_device, &pool_ci, nullptr, &cmd_pool) != VK_SUCCESS) {
        return Err("failed to create command pool for render graph.");
    }

    /* Allocate graph executions ring buffer */
    graphs = new GraphExecution[max_graphs_in_flight] {};
    next_graph = 0u;

    /* Command buffer allocation info */
    VkCommandBufferAllocateInfo cmd_ai { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmd_ai.commandPool = cmd_pool;
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
        if (vkCreateSemaphore(gpu.logical_device, &sema_ci, nullptr, &graphs[i].end_semaphore) != VK_SUCCESS)
            return Err("failed to create end semaphore for graph.");
    }

    /* Initialize the pipeline cache */
    pipeline_cache.init(gpu);

    return Ok();
}

Result<void> RenderGraph::dispatch() {
    /* Get the next graph in the graph executions ring buffer */
    GraphExecution& graph = graphs[next_graph];

    /* In nanoseconds (one second) */
    constexpr uint64_t TIMEOUT = 1'000'000'000u;

    /* Wait for this graph to be out of flight before re-using its resources */
    if (vkWaitForFences(gpu->logical_device, 1u, &graph.flight_fence, true, TIMEOUT) != VK_SUCCESS) {
        return Err("failed while waiting for graph in-flight fence.");
    }
    
    /* Get the render target image index for this graph */
    const bool has_target = target.is_null() == false;
    RenderTargetSlot* rt = nullptr;
    if (has_target) {
        rt = &gpu->get_vram_bank().get_render_target(target);
        if (vkAcquireNextImageKHR(gpu->logical_device, rt->swapchain, TIMEOUT, graph.start_semaphore, VK_NULL_HANDLE, &rt->current_image) != VK_SUCCESS) {
            return Err("failed to acquire next swapchain image for render target.");
        }
    }

    /* Begin recording commands to the graphs command buffer */
    VkCommandBufferBeginInfo cmd_begin { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmd_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(graph.cmd, &cmd_begin) != VK_SUCCESS) {
        return Err("failed to begin recording command buffer for graph.");
    }

    // TODO: Queue all the waves of passes here!!!
    for (u32 s = 0u, w = 0u, e = 0u; e <= waves.size(); ++e) {
        /* If this lane is the start of a new wave */
        if (e == waves.size() || waves[e].wave != w) {
            w++;
            const Result wave_result = queue_wave(graph, s, e);
            if (wave_result.is_err()) return Err(wave_result.unwrap_err());
            s = e;
        }
    }

    // const u32 wave_count = waves[waves.size() - 1u].wave + 1u;
    // for (u32 wave = 0u; wave < wave_count; ++wave) {
    //     for (u32 i = 0u; i < waves.size(); ++i) {
    //         if (waves[i].wave != wave) continue;
            
    //         const Node* node = nodes[waves[i].lane];
    //         printf("[%u] node '%s'\n", wave, node->label.data());
    //     }
    // }

    /* Insert render target pipeline barrier at the end of the command buffer */
    if (has_target) {
        VkImageMemoryBarrier rt_barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        rt_barrier.oldLayout = rt->old_layouts[rt->current_image];
        rt_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        rt->old_layouts[rt->current_image] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        rt_barrier.image = rt->images[rt->current_image];
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
    submit.signalSemaphoreCount = 1u; /* Signal when the work completes */
    submit.pSignalSemaphores = &graph.end_semaphore;
    if (has_target) {
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
        present.pWaitSemaphores = &graph.end_semaphore;
        present.swapchainCount = 1u;
        present.pSwapchains = &rt->swapchain;
        present.pImageIndices = &rt->current_image;

        /* Queue presentation */
        if (vkQueuePresentKHR(gpu->queues.queue_combined, &present) != VK_SUCCESS) {
            return Err("failed to present to render target.");
        }
    }

    /* Update the next graph index */
    if (++next_graph >= max_graphs_in_flight) next_graph = 0u;

    return Ok();
}

Result<void> RenderGraph::queue_wave(GraphExecution& graph, u32 start, u32 end) {
    /* Insert sync barriers for wave descriptors */
    wave_sync_descriptors(*this, start, end);

    /* Queue each node in the wave */
    for (u32 i = start; i < end; ++i) {
        const Node& node = *nodes[waves[i].lane];

        switch (node.type) {
            case NodeType::Compute:
                return queue_compute_node(graph, (const ComputeNode&)node);
            default:
                return Err("unknown node type in render graph.");
        }
    }

    return Ok();
}

Result<void> RenderGraph::queue_compute_node(GraphExecution& graph, const ComputeNode& node) {
    /* Try to get the pipeline for this compute node */
    const Result cache_result = pipeline_cache.get_pipeline(shader_path, node);
    if (cache_result.is_err()) return Err(cache_result.unwrap_err());
    const Pipeline pipeline = cache_result.unwrap();

    /* Bind the compute pipeline */
    vkCmdBindPipeline(graph.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
    const Result push_result = node_push_descriptors(*this, pipeline, node);
    if (push_result.is_err()) return Err(push_result.unwrap_err());

    /* Calculate the dispatch size */
    const u32 dispatch_x = div_up(node.work_x, node.group_x);
    const u32 dispatch_y = div_up(node.work_y, node.group_y);
    const u32 dispatch_z = div_up(node.work_z, node.group_z);

    /* Dispatch the compute pipeline */
    vkCmdDispatch(graph.cmd, dispatch_x, dispatch_y, dispatch_z);

    return Ok();
}

Result<void> RenderGraph::destroy() {
    /* Wait for the queue to idle */
    vkQueueWaitIdle(gpu->queues.queue_combined);

    /* Evict the pipeline cache */
    pipeline_cache.evict();

    /* Destroy graph execution resources */
    vkDestroyCommandPool(gpu->logical_device, cmd_pool, nullptr);
    for (u32 i = 0u; i < max_graphs_in_flight; ++i) {
        vkDestroyFence(gpu->logical_device, graphs[i].flight_fence, nullptr);
        vkDestroySemaphore(gpu->logical_device, graphs[i].start_semaphore, nullptr);
        vkDestroySemaphore(gpu->logical_device, graphs[i].end_semaphore, nullptr);
    }
    delete[] graphs;

    return Ok();
}
