#include "render_graph.hh"

#include <unordered_map>

#include "nodes/compute_node.hh"

#define DEBUG_LOGGING 1

void RenderGraph::new_graph(u32 node_count) {
    /* Clear out the nodes list */
    for (Node* old_node : nodes)
        delete old_node;
    nodes.clear();
    nodes.reserve(node_count);
    waves.clear();
    waves.reserve(node_count);
}

/* Collection of metadata for a graph node. (used during graph compilation) */
struct NodeMeta {
    /* List of versions, one for each dependency. */
    std::vector<u32> versions {};
    /* List of sources (node indices), one for each dependency. */
    std::vector<u32> sources {};
    /* Number of input & output nodes. */
    u32 input_nodes = 0u, output_nodes = 0u;
};

/* Dependency hashmap key. */
inline u32 dependency_key(const Dependency& dep) {
    return dep.resource.raw();
};

Result<void> RenderGraph::end_graph() {
    /* Resource version hashmap (key: handle, value: version) */
    std::unordered_map<u32, u32> version_map {};
    /* Resource source hashmap (key: handle, value: source_node_index) */
    std::unordered_map<u32, u32> source_map {};

    /* Reset the current render target to NULL */
    target = RenderTarget();

    /* Propegate dependency versions through the graph */
    std::vector<NodeMeta> node_meta(nodes.size());
    for (u32 i = 0u; i < nodes.size(); ++i) {
        /* Get the node and its metadata */
        const Node* node = nodes[i];
        NodeMeta& meta = node_meta[i];

        /* Initialize the node metadata */
        meta.versions.resize(node->dependencies.size());
        meta.sources.resize(node->dependencies.size());

        for (u32 j = 0u; j < node->dependencies.size(); ++j) {
            /* Get the dependency and its version */
            const Dependency& dep = node->dependencies[j];
            const u32 key = dependency_key(dep);
            u32& dep_version = meta.versions[j];

            /* Set the version of the dependency from the hashmap */
            dep_version = version_map[key];
            
            /* Record which pass this dependency comes from, UINT32_MAX if first time used */
            if (dep_version == 0u) {
                meta.sources[j] = UINT32_MAX;
            } else {
                meta.sources[j] = source_map[key];
                node_meta[meta.sources[j]].output_nodes += 1u;
                meta.input_nodes += 1u; /* <- increment input node count */
            }

            /* Update the version and source hashmaps in case this resource is written to */
            if (has_flag(dep.flags, DependencyFlags::Readonly) == false) {
                version_map[key] += 1u; /* <- increment version */
                source_map[key] = i; /* <- store source node index */

                /* Save this resource if it is a render target */
                if (dep.resource.get_type() == ResourceType::RenderTarget) {
                    if (target.is_null() == false) return Err("multiple render targets are not allowed in the same graph.");
                    target = (RenderTarget&)dep.resource;
                }
            }
        }
    }

#if DEBUG_LOGGING
    /* Debug logging */
    for (u32 i = 0u; i < nodes.size(); ++i) {
        /* Get the node and its metadata */
        const Node* node = nodes[i];
        const NodeMeta& meta = node_meta[i];

        printf("node '%s'\n", node->label.data());
        printf("~ dependencies: [\n");

        for (u32 j = 0u; j < node->dependencies.size(); ++j) {
            /* Get the dependency and its version */
            const Dependency& dep = node->dependencies[j];
            const u32 id = dependency_key(dep);
            const u32 dep_version = meta.versions[j];
            const u32 dep_source = meta.sources[j];

            /* Log the resource key and version */
            const bool readonly = has_flag(dep.flags, DependencyFlags::Readonly);
            if (readonly) printf("   R 0x%X, v%u", id, dep_version);
            else printf("  RW 0x%X, v%u", id, dep_version);
            
            /* Log whether this resource is a render target */
            const bool is_rt = dep.resource.get_type() == ResourceType::RenderTarget;
            if (is_rt) printf(", [RT]");

            /* Log the source of this resource */
            if (dep_source < nodes.size()) printf(" <- '%s'", nodes[dep_source]->label.data());
            
            printf("\n");
        }
        printf("]");
        printf(" %u -> %u", meta.input_nodes, meta.output_nodes);
        printf("\n");
    }
#endif

    /* Re-use the resource version hashmap */
    version_map.clear();

    /* Compile the nodes into waves using topology sorting */
    for (u32 wave = 0u;; ++wave) {
        /* Find lanes with no unresolved dependencies */
        std::vector<u32> next_wave {};
        for (u32 lane = 0u; lane < nodes.size(); ++lane) {
            /* Get the node and its metadata */
            const Node* node = nodes[lane];
            const NodeMeta& meta = node_meta[lane];

            /* Look for unresolved dependencies in this node */
            bool has_deps = false; 
            for (u32 i = 0u; i < node->dependencies.size(); ++i) {
                const Dependency& dep = node->dependencies[i];

                if (meta.versions[i] == version_map[dependency_key(dep)]) continue;
                has_deps = true;
                break;
            }

            /* Found a node with no unresolved dependencies */
            if (has_deps == false) next_wave.emplace_back(lane);
        }

        if (next_wave.empty()) break; /* No more waves left */

        /* Update the outputs of this new wave for the next wave */
        for (const u32 lane : next_wave) {
            waves.emplace_back(wave, lane);

            /* Get the node and its metadata */
            const Node* node = nodes[lane];
            NodeMeta& meta = node_meta[lane];

            /* Make sure this node won't be included again */
            for (u32& version : meta.versions) version = UINT32_MAX;

            /* Update the resource versions */
            for (u32 i = 0u; i < node->dependencies.size(); ++i) {
                const Dependency& dep = node->dependencies[i];

                if (has_flag(dep.flags, DependencyFlags::Readonly) == false) { 
                    version_map[dependency_key(dep)] += 1;
                }
            }
        }
    }

#if DEBUG_LOGGING
    /* Debug logging */
    const u32 wave_count = waves[waves.size() - 1u].wave + 1u;
    printf("waves: (%u)\n", wave_count);
    for (u32 wave = 0u; wave < wave_count; ++wave) {
        for (u32 i = 0u; i < waves.size(); ++i) {
            if (waves[i].wave != wave) continue;
            
            const Node* node = nodes[waves[i].lane];
            printf("[%u] node '%s'\n", wave, node->label.data());
        }
    }
#endif
    
    return Ok();
}

ComputeNode& RenderGraph::add_compute_pass(std::string_view label, std::string_view shader_path) {
    /* Create the new compute node, and insert it into the nodes list. */
    ComputeNode* new_node = new ComputeNode(label, shader_path);
    nodes.emplace_back((Node*)new_node);
    return *new_node;
}
