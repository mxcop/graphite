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
class RenderGraph : ImplRenderGraph {
    /* List of nodes in the order in which they were queued. */
    std::vector<Node*> nodes {};
    /* Flattened list of waves and their lanes. (output of topology sorting) */
    std::vector<WaveLane> waves {};

public:
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
     * @param file_alias Alias used to find the shader file `shader:my_shader`.
     * @return The new compute node to be customized using the builder pattern.
     */
    ComputeNode& add_compute_pass(std::string_view label, std::string_view file_alias);

    /* Dispatch all the GPU work for the graph, should be called after `end_graph()`. */
    PLATFORM_SPECIFIC Result<void> dispatch(GPUAdapter& gpu);

    /* Destroy the Render Graph, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy(GPUAdapter& gpu);
};
