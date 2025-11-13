#pragma once

/* Interface header */
#include "graphite/render_graph.hh"

#include "graphite/utils/types.hh"
#include "vulkan/api_vk.hh" /* Vulkan API */
#include "wrapper/pipeline_cache_vk.hh"

/* Staging command for a graph execution. */
struct StagingCommand {
    u64 dst_offset = 0u;
    u64 src_offset = 0u;
    u64 bytes = 0u;
    OpaqueHandle dst_resource {};
};

/* The execution data for a graph. */
struct GraphExecution {
    VkCommandBuffer cmd {};
    /* Triggered when the execution can start. (only used when using swapchain) */
    VkSemaphore start_semaphore {};
    /* Indicates whether this execution is in flight. */
    VkFence flight_fence {};
    /* Graph staging buffer allocation. */
    VmaAllocation staging_alloc {};
    /* Graph staging buffer. */
    VkBuffer staging_buffer {};
    /* Graph staging copy commands. */
    std::vector<StagingCommand> staging_commands {};
    u64 staging_stack_ptr = 0u;
};

/**
 * Render Graph.  
 * Used to queue and dispatch a graph of render passes.
 */
class RenderGraph : public AgnRenderGraph {
    /* Shader pipeline cache */
    PipelineCache pipeline_cache {};

    /* Queue all lanes for a given wave. */
    Result<void> queue_wave(const GraphExecution& graph, u32 start, u32 end);

    /* Queue commands for a compute node. */
    Result<void> queue_compute_node(const GraphExecution& graph, const ComputeNode& node);

    /* Queue commands for a rasterisation node */
    Result<void> queue_raster_node(const GraphExecution& graph, const RasterNode& node);

    /* Queue commands to stage graph buffers. */
    void queue_staging(const GraphExecution& graph);

    /* Queue commands to render immediate mode gui. */
    void queue_imgui(const GraphExecution& graph);

public:
    /* Initialize the Render Graph. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu);
    
    /* Upload data to a GPU buffer resource. */
    PLATFORM_SPECIFIC void upload_buffer(Buffer& buffer, const void* data, u64 dst_offset, u64 size);

    /* Dispatch all the GPU work for the graph, should be called after `end_graph()`. */
    PLATFORM_SPECIFIC Result<void> dispatch();

    /* Destroy the Render Graph, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy();

    /* To access nodes and waves. "./wrapper/descriptor_vk.cc" */
    friend Result<void> node_push_descriptors(const RenderGraph& rg, const Pipeline& pipeline, const Node& node);
    friend Result<void> wave_sync_descriptors(const RenderGraph& rg, u32 start, u32 end);
};
