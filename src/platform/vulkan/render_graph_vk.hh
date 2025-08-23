#pragma once

#include "graphite/utils/types.hh"
#include "vulkan/api_vk.hh" /* Vulkan API */
#include "wrapper/pipeline_cache_vk.hh"

/* The execution data for a graph. */
struct GraphExecution {
    VkCommandBuffer cmd {};
    /* Triggered when the execution has finished. */
    VkSemaphore ready_semaphore {};
    /* Indicates whether this execution is in flight. */
    VkFence flight_fence {};
};

struct ImplRenderGraph {
    /* Shader pipeline cache */
    PipelineCache pipeline_cache {};

    /* GPU command pool */
    VkCommandPool cmd_pool {};

    /* List of graph executions */
    GraphExecution* graphs = nullptr;
    u32 next_graph = 0u;
};

/* Interface header */
#include "graphite/render_graph.hh"
