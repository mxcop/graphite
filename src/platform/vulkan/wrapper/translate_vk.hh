#pragma once

#include "vulkan/api_vk.hh"

#include "graphite/nodes/node.hh"

/* Provides functions for translating platform-agnostic types to Vulkan-specific types. */
namespace translate {

/* Convert the platform-agnostic stage flags to Vulkan shader stage flags. */
VkShaderStageFlags stage_flags(DependencyStages stages);

/* Convert the platform-agnostic node type to Vulkan pipeline bind point. */
VkPipelineBindPoint pipeline_bind_point(NodeType node_type);

} /* translate */
