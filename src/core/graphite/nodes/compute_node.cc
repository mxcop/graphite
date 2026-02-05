#include "compute_node.hh"

ComputeNode::ComputeNode(std::string_view label, std::string_view shader_path)
    : Node(label, NodeType::Compute), compute_path(shader_path) {}

ComputeNode::~ComputeNode() {
    if (pc_data != nullptr) delete[] pc_data;
}

ComputeNode& ComputeNode::write(BindHandle resource) {
    /* Insert the write dependency */
    dependencies.emplace_back(resource, DependencyUsage::ReadWrite, DependencyStages::Compute);
    return *this;
}

ComputeNode& ComputeNode::read(BindHandle resource) {
    /* Insert the read dependency */
    dependencies.emplace_back(resource, DependencyUsage::Readonly, DependencyStages::Compute);
    return *this;
}

ComputeNode& ComputeNode::push_constants(void* data, u32 offset, u32 size) { 
    if (pc_data == nullptr) pc_data = new u32[(size + 3u) / 4u];
    memcpy(pc_data, data, size);
    range_offset = offset;
    range_size = size;
    return *this;
}

ComputeNode& ComputeNode::indirect_size(Buffer buffer, u32 offset) {
    /* Insert the read dependency */
    dependencies.emplace_back(buffer, DependencyUsage::IndirectBuffer, DependencyStages::Compute);
    indirect_buffer = buffer;
    return *this;
}
