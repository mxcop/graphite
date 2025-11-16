#pragma once

#include <vector>
#include <string_view>

#include "platform/platform.hh"

#include "resources/handle.hh"
#include "utils/result.hh"
#include "utils/types.hh"

/* Graph wave lane pair. */
struct WaveLane {
    u32 wave = 0u; /* Index of the wave. */
    u32 lane = 0u; /* Index of the node. */
    WaveLane(u32 wave, u32 lane) : wave(wave), lane(lane) {}
};

class GPUAdapter;
class ImGUI;

/* The execution data for a graph. */
PLATFORM_STRUCT struct GraphExecution;

/* Graph nodes. */
class Node;
class ComputeNode;
class RasterNode;

/**
 * @warning Never use this class directly!
 * This is an interface for the platform-specific class.
 */
class AgnRenderGraph {
protected:
    GPUAdapter* gpu = nullptr;
    ImGUI* imgui = nullptr;

    /* List of nodes in the order in which they were queued. */
    std::vector<Node*> nodes {};
    /* Flattened list of waves and their lanes. (output of topology sorting) */
    std::vector<WaveLane> waves {};
    /* Each graph can have up to 1 render target. */
    RenderTarget target {};

    /* Path to load shaders from. */
    std::string shader_path = ".";

    /* Maximum number of graphs in flight. */
    u32 max_graphs_in_flight = 1u;
    /* Staging memory limit per graph in flight. */
    u64 graph_staging_limit = 65536u;

    /* List of graph executions */
    GraphExecution* graphs = nullptr;
    u32 active_graph_index = 0u;
    
    /* Get a reference to the active graph execution. */
    const GraphExecution& active_graph() const;
    /* Get a reference to the active graph execution. */
    GraphExecution& active_graph();
    /* Move on to the next graph execution, should be called at the end of `dispatch()`. */
    inline void next_graph() { if (++active_graph_index >= max_graphs_in_flight) active_graph_index = 0u; };

    /* Wait until it's safe to create a new graph. */
    PLATFORM_SPECIFIC Result<void> wait_until_safe() = 0;

public:
    /* Set the path from which to load shader files. (default: `"."`) */
    void set_shader_path(std::string path) { shader_path = path; };
    /* Set the maximum number of graphs in flight. (default: `1`) */
    void set_max_graphs_in_flight(u32 max) { max_graphs_in_flight = max; };
    /* Set the staging memory limit per graph in flight. (default: `65536`) */
    void set_staging_limit(u64 bytes) { graph_staging_limit = bytes; };
    /* Add an immediate mode gui to this render graph. (only works when rendering to a render target) */
    void add_imgui(ImGUI& gui) { imgui = &gui; };

    /* Initialize the Render Graph. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu) = 0;

    /**
     * @brief Start a new graph, this clear the graph.
     * 
     * @param node_count Expected number of nodes in the graph.
     */
    Result<void> new_graph(u32 node_count = 128u);

    /**
     * @brief End the current graph, this will sort & compile the graph.
     */
    Result<void> end_graph();

    /**
     * @brief Add a compute pass to the render graph.
     * 
     * @param label Label for this pass. (should be unique)
     * @param shader_path Path to the shader file relative to the shader path set using `set_shader_path(...)`.
     * @return The new compute node to be customized using the builder pattern.
     */
    ComputeNode& add_compute_pass(std::string_view label, std::string_view shader_path);

    /**
     * @brief Add a rasterisation pass to the render graph.
     *
     * @param label Label for this pass. (should be unique)
     * @param vx_path Path to the vertex shader file relative to the shader path set using `set_shader_path(...)`.
     * @param px_path Path to the pixel shader file relative to the shader path set using `set_shader_path(...)`.
     * @return The new compute node to be customized using the builder pattern.
     */
    RasterNode& add_raster_pass(std::string_view label, std::string_view vx_path, std::string_view px_path);
    
    /* Upload data to a GPU buffer resource. */
    PLATFORM_SPECIFIC void upload_buffer(Buffer& buffer, const void* data, u64 dst_offset, u64 size) = 0;

    /* Dispatch all the GPU work for the graph, should be called after `end_graph()`. */
    PLATFORM_SPECIFIC Result<void> dispatch() = 0;

    /* De-initialize the Render Graph, free all its resources. */
    PLATFORM_SPECIFIC Result<void> deinit() = 0;
};

#include PLATFORM_INCLUDE(render_graph)
