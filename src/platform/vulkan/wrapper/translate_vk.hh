#pragma once

#include "vulkan/api_vk.hh"

#include "graphite/nodes/node.hh"
#include "graphite/nodes/raster_node.hh"

#include "graphite/resources/buffer.hh"
#include "graphite/resources/texture.hh"
#include "graphite/resources/sampler.hh"

/* Provides functions for translating platform-agnostic types to Vulkan-specific types. */
namespace translate {

/* Convert the platform-agnostic stage flags to Vulkan shader stage flags. */
VkShaderStageFlags stage_flags(DependencyStages stages);

/* Convert the platform-agnostic texture usage to a desired image layout. */
VkImageLayout desired_image_layout(TextureUsage usage, DependencyFlags flags);

/* Convert the platform-agnostic texture usage to Vulkan image descriptor type. */
VkDescriptorType image_descriptor_type(TextureUsage usage, DependencyFlags flags);

/* Convert the platform-agnostic dependency flags to a desired image descriptor type. */
VkDescriptorType desired_image_type(DependencyFlags flags);

/* Convert the platform-agnostic node type to Vulkan pipeline bind point. */
VkPipelineBindPoint pipeline_bind_point(NodeType node_type);

/* Convert the platform-agnostic buffer usage to buffer usage flags. */
VkBufferUsageFlags buffer_usage(BufferUsage usage);

/* Convert the platform-agnostic buffer usage to Vulkan buffer descriptor type. */
VkDescriptorType buffer_descriptor_type(BufferUsage usage);

/* Convert the platform-agnostic texture format to Vulkan texture format. */
VkFormat texture_format(TextureFormat format);

/* Convert the platform-agnostic texture usage to texture usage flags. */
VkImageUsageFlags texture_usage(TextureUsage usage);

/* Convert the platform-agnostic filter to Vulkan sampler filter. */
VkFilter sampler_filter(Filter filter);

/* Convert the platform-agnostic address mode to Vulkan sampler address mode. */
VkSamplerAddressMode sampler_address_mode(AddressMode mode);

/* Convert the platform-agnostic border color to Vulkan sampler border color. */
VkBorderColor sampler_border_color(BorderColor color);

/* Get the number of bytes per vertex attribute for a given vertex attribute format. */
u32 vertex_attribute_size(const AttrFormat fmt);

/* Convert the platform-agnostic vertex attribute format to a vertex format. */
VkFormat vertex_format(const AttrFormat fmt);

/* Convert the platform-agnostic primitive topology. */
VkPrimitiveTopology primitive_topology(const Topology topology);

/* Convert the platform-agnostic load operation. */
VkAttachmentLoadOp load_operation(const LoadOp op);

/* Convert the platform-agnostic vertex input rate. */
VkVertexInputRate vertex_input_rate(const VertexInputRate rate);

} /* translate */
