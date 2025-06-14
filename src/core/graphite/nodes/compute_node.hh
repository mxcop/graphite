#pragma once

#include <string_view>

#include "node.hh"

class RenderTarget;

/**
 * Render Graph Compute Node.  
 * Used to build a compute shader pass.
 */
class ComputeNode : Node {
    /* Compute shader file alias */
    std::string_view compute_sfa {};

    u32 group_x = 1u, group_y = 1u, group_z = 1u; /* Thread group size */
    u32 work_x = 1u, work_y = 1u, work_z = 1u; /* Work size */
    
    /* Can only be constructed by the RenderGraph */
    ComputeNode() = default;
    ComputeNode(std::string_view label, std::string_view file_alias);

public:
    /* No copies allowed */
    ComputeNode(const ComputeNode&) = delete;
    ComputeNode& operator=(const ComputeNode&) = delete;

    /* Add a render target as an output for this node. */
    ComputeNode& write(RenderTarget& render_target);

    /* Set the thread group size for this node. */
    inline ComputeNode& group_size(u32 x, u32 y = 1u, u32 z = 1u) { group_x = x; group_y = y; group_z = z; return *this; }

    /* Set the work size for this node. (this will be divided by the `group_size` to get the dispatch size) */
    inline ComputeNode& work_size(u32 x, u32 y = 1u, u32 z = 1u) { work_x = x; work_y = y; work_z = z; return *this; }

    friend class RenderGraph;
};
