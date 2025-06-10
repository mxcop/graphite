#pragma once

#include <string_view>

#include "node.hh"

/**
 * Render Graph Compute Node.  
 * Used to build a compute shader pass.
 */
class ComputeNode : Node {
    /* Compute shader file alias. */
    std::string_view compute_sfa {};
    
    /* Can only be constructed by the RenderGraph */
    ComputeNode() = default;
    ComputeNode(std::string_view label, std::string_view file_alias);

public:
    /* No copies allowed */
    ComputeNode(const ComputeNode&) = delete;
    ComputeNode& operator=(const ComputeNode&) = delete;

    /* Add a resource as an output for this node. */
    ComputeNode& write(BindHandle resource);

    friend class RenderGraph;
};
