#pragma once

#include <string_view>

#include "node.hh"

/**
 * Render Graph Compute Node.  
 * Used to build a compute shader pass.
 */
class ComputeNode : public Node {
    /* Can only be constructed by the RenderGraph */
    ComputeNode() = default;
    ComputeNode(std::string_view label, std::string_view shader_path);

public:
    /* Compute shader file path */
    std::string_view compute_path {};

    u32 group_x = 1u, group_y = 1u, group_z = 1u; /* Thread group size */
    u32 work_x = 1u, work_y = 1u, work_z = 1u; /* Work size */
    
    /* Indirect Dispatch. */
    Buffer indirect_buffer {};
    uint32_t indirect_offset = 0u;

    /* No copies allowed */
    ComputeNode(const ComputeNode&) = delete;
    ComputeNode& operator=(const ComputeNode&) = delete;

    /* Add a bindable resource as an output for this node. */
    ComputeNode& write(BindHandle resource);

    /* Add a bindable resource as an input for this node. */
    ComputeNode& read(BindHandle resource);

    /* Set the thread group size for this node. */
    inline ComputeNode& group_size(u32 x, u32 y = 1u, u32 z = 1u) { group_x = x; group_y = y; group_z = z; return *this; }

    /* Set the work size for this node. (this will be divided by the `group_size` to get the dispatch size) */
    inline ComputeNode& work_size(u32 x, u32 y = 1u, u32 z = 1u) { work_x = x; work_y = y; work_z = z; return *this; }

    /* Set the indirect_buffer which will be used to get the dispatch args buffer for the vkCmdDispatchIndirect call. */
    inline ComputeNode& indirect_size(Buffer buffer) { indirect_buffer = buffer; return *this; }

    /* To access constructors */
    friend class AgnRenderGraph;
};
