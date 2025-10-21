#pragma once

/* Interface header */
#include "graphite/render_graph.hh"

#include "graphite/utils/types.hh"
#include "vulkan/api_vk.hh" /* Vulkan API */
#include "wrapper/pipeline_cache_vk.hh"

/* The execution data for a graph. */
struct GraphExecution {
    VkCommandBuffer cmd {};
    /* Triggered when the execution can start. (only used when using swapchain) */
    VkSemaphore start_semaphore {};
    /* Triggered when the execution has finished. */
    VkSemaphore end_semaphore {};
    /* Indicates whether this execution is in flight. */
    VkFence flight_fence {};
};

/**
 * Render Graph.  
 * Used to queue and dispatch a graph of render passes.
 */
class RenderGraph : public AgnRenderGraph {
    /* Shader pipeline cache */
    PipelineCache pipeline_cache {};

    /* GPU command pool */
    VkCommandPool cmd_pool {};

    /* Queue all lanes for a given wave. */
    Result<void> queue_wave(const GraphExecution& graph, u32 start, u32 end);

    /* Queue commands for a compute node. */
    Result<void> queue_compute_node(const GraphExecution& graph, const ComputeNode& node);

public:
    /* Initialize the Render Graph. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu);

    /* Dispatch all the GPU work for the graph, should be called after `end_graph()`. */
    PLATFORM_SPECIFIC Result<void> dispatch();

    /* Destroy the Render Graph, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy();

    /* To access nodes and waves. "./wrapper/descriptor_vk.cc" */
    friend Result<void> node_push_descriptors(const RenderGraph& rg, const Pipeline& pipeline, const Node& node);
    friend Result<void> wave_sync_descriptors(const RenderGraph& rg, u32 start, u32 end);
};
