#include "render_graph.hh"

#include "nodes/compute_node.hh"

void RenderGraph::new_graph(u32 node_count) {
    /* Clear out the nodes list */
    for (Node* old_node : nodes)
        delete old_node;
    nodes.clear();
    nodes.reserve(node_count);
}

Result<void> RenderGraph::end_graph() {
    return Ok();
}

ComputeNode &RenderGraph::add_compute_pass(std::string_view label, std::string_view file_alias) {
    /* Create the new compute node, and insert it into the nodes list. */
    ComputeNode* new_node = new ComputeNode(label, file_alias);
    nodes.emplace_back((Node*)new_node);
    return *new_node;
}
