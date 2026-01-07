#pragma once

#include <vector>
#include <string_view>
#include <variant>

#include "graphite/resources/handle.hh"
#include "graphite/utils/enum_flags.hh"
#include "graphite/utils/types.hh"

/* Render Graph resource dependency stages. */
enum class DependencyStages : u32 {
    None = 0u,
    Compute = 1u << 0u,
    Vertex = 1u << 1u,
    Pixel = 1u << 2u,
};
ENUM_CLASS_FLAGS(DependencyStages);
using ShaderStages = DependencyStages;

/* Render Graph resource dependency usage. */
enum class DependencyUsage : u32 {
    None = 0u,
    /* Read-only */
    VertexBuffer,
    IndirectBuffer,
    Readonly,
    /* Read/Write */
    ReadWrite,
    ColorAttachment,
};

/* Render Graph resource dependency. */
struct Dependency {
    BindHandle resource {};
    DependencyUsage usage = DependencyUsage::None;
    DependencyStages stages {};
    u32 source_node {};
    u32 source_index {};

    /* Returns true if this dependency is read-only. */
    bool is_readonly() const;
    /* Returns true if this dependency should be unbound. */
    bool is_unbound() const;

    Dependency(BindHandle resource, DependencyUsage usage, DependencyStages stages);
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
