#include "compute_node.hh"

ComputeNode::ComputeNode(std::string_view label, std::string_view file_alias) : Node(label), compute_sfa(file_alias) {}
