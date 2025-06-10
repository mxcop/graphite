#pragma once

#include <vector>
#include <string_view>

#include "graphite/resources/handle.hh"
#include "graphite/utils/enum_flags.hh"

/* Render Graph resource dependency flags. */
enum class DependencyFlags : uint32_t {
    None = 0u,
    Readonly = 1u << 0u,   /* The dependency is read only. */
    Attachment = 1u << 1u, /* The dependency is used as an attachment. */
};
ENUM_CLASS_FLAGS(DependencyFlags);

/* Render Graph resource dependency. */
struct Dependency {
    BindHandle resource {};
    DependencyFlags flags {};

    Dependency(BindHandle resource, DependencyFlags flags);
};

/**
 * Render Graph Node.  
 * A base for all other shader pass nodes.
 */
class Node {
public:
    /* Unique label. (primarily for debugging) */
    std::string_view label {}; 

    Node() = delete;

protected:
    /* List of node resource dependencies. */
    std::vector<Dependency> dependencies {};

    Node(std::string_view label);
};
