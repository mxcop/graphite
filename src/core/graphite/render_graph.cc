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

/* Collection of metadata for a graph node. (used during topology sort) */
struct NodeMeta {
    /* List of versions, one for each dependency. */
    std::vector<u32> versions {};
    /* List of sources, one for each dependency. */
    std::vector<u32> sources {};
    /* Number of source nodes. */
    u32 source_count = 0u;
};

/* Dependency hashmap key. */
inline u32 dependency_key(const Dependency& dep) {
    return dep.resource.raw();
};

Result<void> RenderGraph::end_graph() {

    /* TODO: Figure out all dependencies between graph nodes. */
    
    /* Resource version hashmap (key: handle, value: version) */
    std::unordered_map<u32, u32> version_map {};
    /* Resource source hashmap (key: handle, value: source_node_index) */
    std::unordered_map<u32, u32> source_map {};

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
                meta.source_count += 1u; /* <- increment source node count */
            }

            /* Update the version and source hashmaps in case this resource is written to */
            if (has_flag(dep.flags, DependencyFlags::Readonly) == false) {
                version_map[key] += 1u; /* <- increment version */
                source_map[key] = i; /* <- store source node index */
            }
        }
    }

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
        printf("]\n");
    }
    
    /* Find node clusters in the graph */
    for (u32 i = 0u, s = 0u, c = 0u; i < nodes.size(); ++i) {
        /* Get the node and its metadata */
        const Node* node = nodes[i];
        const NodeMeta& meta = node_meta[i];

        bool should_split = false;
        for (u32 j = i; j < nodes.size(); ++j) {
            for (u32 k = 0u; k < nodes[j]->dependencies.size(); ++k) {
                if (node_meta[j].sources[k] == i && node_meta[j].source_count > 1u) { 
                    should_split = true;
                }
            }
        }
        // bool  = false;
        // for (u32 j = 0u; j < node->dependencies.size(); ++j) {
        //     if (meta.sources[j] == UINT32_MAX) continue;
        //     if (node_meta[meta.sources[j]].source_count > 1u) previous_source = false;
        // }

        /* Continue in this cluster if there's only 1 source */
        if (meta.source_count <= 1u && should_split == false) continue;

        printf("cluster %u: %u..=%u\n", c, s, i);

        s = i + 1; /* <- save the new cluster start index */
        c += 1u;
    }


    return Ok();
}

ComputeNode& RenderGraph::add_compute_pass(std::string_view label, std::string_view file_alias) {
    /* Create the new compute node, and insert it into the nodes list. */
    ComputeNode* new_node = new ComputeNode(label, file_alias);
    nodes.emplace_back((Node*)new_node);
    return *new_node;
}
