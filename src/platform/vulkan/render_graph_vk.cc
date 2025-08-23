#include "render_graph_vk.hh"

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
        if (vkCreateSemaphore(gpu.logical_device, &sema_ci, nullptr, &graphs[i].ready_semaphore) != VK_SUCCESS)
            return Err("failed to create ready semaphore for graph.");
    }

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

    /* Begin recording commands to the graphs command buffer */
    VkCommandBufferBeginInfo cmd_begin { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmd_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(graph.cmd, &cmd_begin) != VK_SUCCESS) {
        return Err("failed to begin recording command buffer for graph.");
    }

    // TODO: Queue all the waves of passes here!!!

    /* Finish recording commands to the graphs command buffer */
    vkEndCommandBuffer(graph.cmd);

    /* Reset the in-flight fence */
    if (vkResetFences(gpu->logical_device, 1u, &graph.flight_fence) != VK_SUCCESS) {
        return Err("failed to reset graph in-flight fence.");
    }

    /* Graph submission info */
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo submit { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    // submit.waitSemaphoreCount = 1u; /* Wait for image acquired */
    // submit.pWaitSemaphores = &frame.image_acquired;
    // TODO: Add render target semaphores here ^^^
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1u;
    submit.pCommandBuffers = &graph.cmd;
    submit.signalSemaphoreCount = 1u; /* Signal when the work completes */
    submit.pSignalSemaphores = &graph.ready_semaphore;

    /* Submit the graph commands to the queue */
    if (vkQueueSubmit(gpu->queues.queue_combined, 1u, &submit, graph.flight_fence) != VK_SUCCESS) {
        return Err("failed to submit graph commands.");
    }

    // TODO: Queue presentation for all render targets involved in graph.

    next_graph++;
    return Ok();
}

Result<void> RenderGraph::destroy() {
    /* Wait for the queue to idle */
    vkQueueWaitIdle(gpu->queues.queue_combined);

    /* Destroy graph execution resources */
    vkDestroyCommandPool(gpu->logical_device, cmd_pool, nullptr);
    for (u32 i = 0u; i < max_graphs_in_flight; ++i) {
        vkDestroyFence(gpu->logical_device, graphs[i].flight_fence, nullptr);
        vkDestroySemaphore(gpu->logical_device, graphs[i].ready_semaphore, nullptr);
    }
    delete[] graphs;

    return Ok();
}
