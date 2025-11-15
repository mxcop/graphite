#pragma once

#include <vector>
#include <string_view>
#include <variant>

#include "graphite/resources/handle.hh"
#include "graphite/utils/enum_flags.hh"
#include "graphite/utils/types.hh"

/* Render Graph resource dependency flags. */
enum class DependencyFlags : u32 {
    None = 0u,
    Readonly = 1u << 0u,   /* The dependency is read only. */
    Attachment = 1u << 1u, /* The dependency is used as an attachment. */
    Unbound = 1u << 2u,    /* The dependency is unbound (ex: Vertex Buffer). */
};
ENUM_CLASS_FLAGS(DependencyFlags);

/* Render Graph resource dependency stages. */
enum class DependencyStages : u32 {
    None = 0u,
    Compute = 1u << 0u,
    Vertex = 1u << 1u,
    Pixel = 1u << 2u,
};
ENUM_CLASS_FLAGS(DependencyStages);
using ShaderStages = DependencyStages;

/* Render Graph resource dependency. */
struct Dependency {
    BindHandle resource {};
    DependencyFlags flags {};
    DependencyStages stages {};

    Dependency(BindHandle resource, DependencyFlags flags, DependencyStages stages);
};

/* Render Graph node type. */
enum class NodeType : u32 {
    Invalid = 0u,
    Compute = 1u, /* Compute pass node. */
    Raster = 2u,  /* Rasterisation pass node. */
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

    /* List of node resource dependencies. */
    std::vector<Dependency> dependencies {};

    Node() = delete;
    Node(std::string_view label, NodeType type);

    virtual ~Node() = default;
};
