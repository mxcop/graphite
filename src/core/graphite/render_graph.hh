#pragma once

#include <vector>
#include <string_view>

#include "platform/platform.hh"

#include "gpu_adapter.hh"
#include "utils/result.hh"
#include "utils/types.hh"

/* Graph wave lane pair. */
struct WaveLane {
    u32 wave = 0u; /* Index of the wave. */
    u32 lane = 0u; /* Index of the node. */
    WaveLane(u32 wave, u32 lane) : wave(wave), lane(lane) {}
};

/* Include platform-specific Impl struct */
PLATFORM_SPECIFIC struct ImplRenderGraph;
#include PLATFORM_H(render_graph)

/* Graph nodes. */
class Node;
class ComputeNode;

/**
 * Render Graph.  
 * Used to queue and dispatch a graph of render passes.
 */
class RenderGraph : public ImplRenderGraph {
    GPUAdapter* gpu = nullptr;

    /* List of nodes in the order in which they were queued. */
    std::vector<Node*> nodes {};
    /* Flattened list of waves and their lanes. (output of topology sorting) */
    std::vector<WaveLane> waves {};

    /* Path to load shaders from. */
    std::string shader_path = ".";

    /* Maximum number of graphs in flight. */
    u32 max_graphs_in_flight = 1u;

public:
    /* Set the path from which to load shader files. (default: `"."`) */
    void set_shader_path(std::string path) { shader_path = path; };
    /* Set the maximum number of graphs in flight. (default: `1`) */
    void set_max_graphs_in_flight(u32 max) { max_graphs_in_flight = max; };

    /* Initialize the Render Graph. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu);

    /**
     * @brief Start a new graph, this clear the graph.
     * 
     * @param node_count Expected number of nodes in the graph.
     */
    void new_graph(u32 node_count = 128u);

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

    /* Dispatch all the GPU work for the graph, should be called after `end_graph()`. */
    PLATFORM_SPECIFIC Result<void> dispatch();

    /* Destroy the Render Graph, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy();
};
