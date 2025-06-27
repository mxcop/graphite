#include "compute_node.hh"

ComputeNode::ComputeNode(std::string_view label, std::string_view file_alias) : Node(label, NodeType::Compute), compute_sfa(file_alias) {}

ComputeNode& ComputeNode::write(BindHandle& resource) {
    /* Insert the write dependency */
    dependencies.emplace_back(resource, DependencyFlags::None);
    return *this;
}
