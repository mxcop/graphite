#pragma once

#include "vulkan/api_vk.hh"

#include "graphite/nodes/node.hh"

#include "graphite/resources/buffer.hh"

/* Provides functions for translating platform-agnostic types to Vulkan-specific types. */
namespace translate {

/* Convert the platform-agnostic stage flags to Vulkan shader stage flags. */
VkShaderStageFlags stage_flags(DependencyStages stages);

/* Convert the platform-agnostic dependency flags to a desired image layout. */
VkImageLayout desired_image_layout(DependencyFlags flags);

/* Convert the platform-agnostic dependency flags to a desired image descriptor type. */
VkDescriptorType desired_image_type(DependencyFlags flags);

/* Convert the platform-agnostic node type to Vulkan pipeline bind point. */
VkPipelineBindPoint pipeline_bind_point(NodeType node_type);

/* Convert the platform-agnostic buffer usage to buffer usage flags. */
VkBufferUsageFlags buffer_usage(const BufferUsage usage);

} /* translate */
