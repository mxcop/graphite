#include "node.hh"

Dependency::Dependency(BindHandle resource, DependencyUsage usage, DependencyStages stages)
    : resource(resource), usage(usage), stages(stages) {}

Node::Node(std::string_view label, NodeType type) : label(label), type(type) {
    /* Reserve space for at least 12 dependencies */
    dependencies.reserve(12);
}

bool Dependency::is_readonly() const {
    switch (usage) {
    /* Read-only */
    case DependencyUsage::VertexBuffer:
    case DependencyUsage::IndirectBuffer:
    case DependencyUsage::Readonly:
        return true;

    /* Read/Write */
    case DependencyUsage::ReadWrite:
    case DependencyUsage::ColorAttachment:
    case DependencyUsage::DepthStencil:
        return false;
    }
    return false;
}

bool Dependency::is_unbound() const {
    switch (usage) {
    /* Unbound */
    case DependencyUsage::VertexBuffer:
    case DependencyUsage::IndirectBuffer:
    case DependencyUsage::ColorAttachment:
    case DependencyUsage::DepthStencil:
        return true;

    /* Bound */
    case DependencyUsage::Readonly:
    case DependencyUsage::ReadWrite:
        return false;
    }
    return false;
}
