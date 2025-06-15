#include "render_graph.hh"

#include <unordered_map>

#include "nodes/compute_node.hh"

void RenderGraph::new_graph(u32 node_count) {
    /* Clear out the nodes list */
    for (Node* old_node : nodes)
        delete old_node;
    nodes.clear();
    nodes.reserve(node_count);
}

/* Collection of versions for a graph node. (used during topology sort) */
struct Versions {
    std::vector<u32> dependencies {};
};

/* Dependency hashmap key. */
inline u64 dependency_key(const Dependency& dep) {
    if (dep.is_render_target()) {
        return dep.resource.raw() | 0xFFFFFFFF00000000u;
    }
    return dep.resource.raw();
};

Result<void> RenderGraph::end_graph() {

    /* TODO: Figure out all dependencies between graph nodes. */
    
    /* Resource version hashmap (key: handle/ptr, value: version) */
    std::unordered_map<u64, u32> version_map {};

    /* Propegate dependency versions through the graph */
    std::vector<Versions> node_versions(nodes.size());
    for (u32 i = 0u; i < nodes.size(); ++i) {
        /* Get the node and its versions */
        const Node* node = nodes[i];
        Versions& versions = node_versions[i];
        versions.dependencies.resize(node->dependencies.size());

        for (u32 j = 0u; j < node->dependencies.size(); ++j) {
            /* Get the dependency and its version */
            const Dependency& dep = node->dependencies[j];
            const u64 key = dependency_key(dep);
            u32& dep_version = versions.dependencies[j];

            /* Set the version of this dependency in the graph */
            if (has_flag(dep.flags, DependencyFlags::Readonly)) {
                dep_version = version_map[key];
            } else {
                dep_version = version_map[key];
                version_map[key] += 1u; /* <- increment version */
            }
        }
    }

    /* Debug logging */
    for (u32 i = 0u; i < nodes.size(); ++i) {
        /* Get the node and its versions */
        const Node* node = nodes[i];
        Versions& versions = node_versions[i];

        printf("node '%s'\n", node->label.data());
        printf("~ dependencies: [\n");

        for (u32 j = 0u; j < node->dependencies.size(); ++j) {
            /* Get the dependency and its version */
            const Dependency& dep = node->dependencies[j];
            const u32 id = dependency_key(dep);
            u32& dep_version = versions.dependencies[j];

            /* Log the resource key and version */
            const bool readonly = has_flag(dep.flags, DependencyFlags::Readonly);
            if (readonly) printf("   R 0x%X, v%u", id, dep_version);
            else printf("  RW 0x%X, v%u", id, dep_version);
            
            /* Log whether this resource is a render target */
            const bool is_rt = dep.is_render_target();
            if (is_rt) printf(", [RT]");
            
            printf("\n");
        }
        printf("]\n");
    }

    return Ok();
}

ComputeNode& RenderGraph::add_compute_pass(std::string_view label, std::string_view file_alias) {
    /* Create the new compute node, and insert it into the nodes list. */
    ComputeNode* new_node = new ComputeNode(label, file_alias);
    nodes.emplace_back((Node*)new_node);
    return *new_node;
}
