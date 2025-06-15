#pragma once

#include <vector>
#include <string_view>
#include <variant>

#include "graphite/resources/handle.hh"
#include "graphite/utils/enum_flags.hh"
#include "graphite/utils/types.hh"

class RenderTarget;

/* Render Graph resource dependency flags. */
enum class DependencyFlags : u32 {
    None = 0u,
    RenderTarget = 1u << 0u, /* The dependency is a render target. */
    Readonly = 1u << 1u,     /* The dependency is read only. */
    Attachment = 1u << 2u,   /* The dependency is used as an attachment. */
};
ENUM_CLASS_FLAGS(DependencyFlags);

/* Render Graph resource dependency. */
struct Dependency {
    BindHandle resource {};
    DependencyFlags flags {};

    Dependency(BindHandle resource, DependencyFlags flags);
    Dependency(RenderTarget& render_target, DependencyFlags flags);

    /* Is this a dependency on a render target resource? */
    inline bool is_render_target() const { return has_flag(flags, DependencyFlags::RenderTarget); }
};

/* Render Graph node type. */
enum class NodeType : u32 {
    Invalid = 0u,
    Compute = 1u, /* Compute pass node. */
};

/**
 * Render Graph Node.  
 * A base for all other shader pass nodes.
 */
class Node {
public:
    /* Unique label. (primarily for debugging) */
    std::string_view label {}; 
    /* Node type. */
    NodeType type = NodeType::Invalid;

    Node() = delete;

protected:
    /* List of node resource dependencies. */
    std::vector<Dependency> dependencies {};

    Node(std::string_view label, NodeType type);

    friend class RenderGraph;
};
