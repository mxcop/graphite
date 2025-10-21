#include "compute_node.hh"

ComputeNode::ComputeNode(std::string_view label, std::string_view shader_path) : Node(label, NodeType::Compute), compute_path(shader_path) {}

ComputeNode& ComputeNode::write(BindHandle resource) {
    /* Insert the write dependency */
    dependencies.emplace_back(resource, DependencyFlags::None, DependencyStages::Compute);
    return *this;
}

ComputeNode& ComputeNode::read(BindHandle resource) {
    /* Insert the read dependency */
    dependencies.emplace_back(resource, DependencyFlags::Readonly, DependencyStages::Compute);
    return *this;
}
