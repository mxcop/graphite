#include "compute_node.hh"

ComputeNode::ComputeNode(std::string_view label, std::string_view file_alias) : Node(label), compute_sfa(file_alias) {}

ComputeNode& ComputeNode::write(RenderTarget& render_target) {
    /* Insert the write dependency */
    dependencies.emplace_back(render_target, DependencyFlags::None);
    return *this;
}
