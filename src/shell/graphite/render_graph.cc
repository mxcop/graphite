#include "render_graph.hh"

#include "nodes/compute_node.hh"

ComputeNode& RenderGraph::add_compute_pass(std::string_view label, std::string_view file_alias) {
    /* Create the new compute node, and insert it into the nodes list. */
    ComputeNode* new_node = new ComputeNode(label, file_alias);
    nodes.emplace_back(new_node);
    return *new_node;
}
