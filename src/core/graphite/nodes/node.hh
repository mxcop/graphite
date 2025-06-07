#pragma once

#include <string_view>

/**
 * Render Graph Node.  
 * A base for all other shader pass nodes.
 */
class Node {
public:
    std::string_view label {}; /* Unique label. */

    Node() = delete;
protected:
    Node(std::string_view label);
};
