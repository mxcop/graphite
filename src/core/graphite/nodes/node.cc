#include "node.hh"

#include "graphite/render_target.hh"

Dependency::Dependency(BindHandle resource, DependencyFlags flags)
    : resource(resource), flags(flags) {}

Dependency::Dependency(RenderTarget &render_target, DependencyFlags flags)
    : resource(render_target.handle), flags(flags | DependencyFlags::RenderTarget) {}

Node::Node(std::string_view label, NodeType type) : label(label), type(type) {
    /* Reserve space for at least 12 dependencies */
    dependencies.reserve(12);
}
