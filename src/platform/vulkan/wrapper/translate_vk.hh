#pragma once

#include "vulkan/api_vk.hh"

#include "graphite/nodes/compute_node.hh"

/* Provides functions for translating platform-agnostic types to Vulkan-specific types. */
namespace translate {

/* Convert the platform-agnostic stage flags to Vulkan shader stage flags. */
VkShaderStageFlags stage_flags(DependencyStages stages);

} /* translate */
