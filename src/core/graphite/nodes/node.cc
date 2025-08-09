#include "node.hh"

Dependency::Dependency(BindHandle resource, DependencyFlags flags, DependencyStages stages)
    : resource(resource), flags(flags), stages(stages) {}

Node::Node(std::string_view label, NodeType type) : label(label), type(type) {
    /* Reserve space for at least 12 dependencies */
    dependencies.reserve(12);
}
