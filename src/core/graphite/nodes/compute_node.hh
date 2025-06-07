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

public:
    ComputeNode() = default;
    ComputeNode(std::string_view label, std::string_view file_alias);
};
