#include "node.hh"

Dependency::Dependency(BindHandle resource, DependencyFlags flags)
    : resource(resource), flags(flags) {}

Dependency::Dependency(RenderTarget &render_target, DependencyFlags flags)
    : resource(&render_target), flags(flags | DependencyFlags::RenderTarget) {}

Node::Node(std::string_view label) : label(label) {
    /* Reserve space for at least 12 dependencies */
    dependencies.reserve(12);
}
