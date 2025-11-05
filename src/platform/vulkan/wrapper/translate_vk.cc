#include "translate_vk.hh"

namespace translate {

/* Convert the platform-agnostic stage flags to Vulkan shader stage flags. */
VkShaderStageFlags stage_flags(DependencyStages stages) {
    VkShaderStageFlags flags = 0x00;
    if (has_flag(stages, DependencyStages::Compute)) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    if (has_flag(stages, DependencyStages::Vertex)) flags |= VK_SHADER_STAGE_VERTEX_BIT;
    if (has_flag(stages, DependencyStages::Pixel)) flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    return flags;
}

/* Convert the platform-agnostic dependency flags to a desired image layout. */
VkImageLayout desired_image_layout(DependencyFlags flags) {
    if (has_flag(flags, DependencyFlags::Attachment)) return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    if (has_flag(flags, DependencyFlags::Readonly)) return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
    return VK_IMAGE_LAYOUT_GENERAL;
}

/* Convert the platform-agnostic dependency flags to a desired image descriptor type. */
VkDescriptorType desired_image_type(DependencyFlags flags) {
    if (has_flag(flags, DependencyFlags::Readonly)) return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
}

/* Convert the platform-agnostic node type to Vulkan pipeline bind point. */
VkPipelineBindPoint pipeline_bind_point(NodeType node_type) {
    switch (node_type) {
        case NodeType::Compute:
            return VK_PIPELINE_BIND_POINT_COMPUTE;
        case NodeType::Raster:
            return VK_PIPELINE_BIND_POINT_GRAPHICS;
        default:
            return VK_PIPELINE_BIND_POINT_MAX_ENUM;
    }
}

VkBufferUsageFlags buffer_usage(const BufferUsage usage) {
    VkBufferUsageFlags flags = 0x00;
    if (has_flag(usage, BufferUsage::TransferDst)) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (has_flag(usage, BufferUsage::TransferSrc)) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (has_flag(usage, BufferUsage::Constant)) flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (has_flag(usage, BufferUsage::Storage)) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (has_flag(usage, BufferUsage::Vertex)) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    //if (has_flag(usage, BufferUsage::eIndex)) flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    //if (has_flag(usage, BufferUsage::eIndirect)) flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    return flags;
}

VkDescriptorType buffer_descriptor_type(const BufferUsage usage) {
    if (has_flag(usage, BufferUsage::Constant)) return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    if (has_flag(usage, BufferUsage::Storage)) return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

u32 vertex_attribute_size(const AttrFormat fmt) {
    switch (fmt) {
        case AttrFormat::X32_SFloat:
            return 4u;
        case AttrFormat::XY32_SFloat:
            return 8u;
        case AttrFormat::XYZ32_SFloat:
            return 12u;
        case AttrFormat::XYZW32_SFloat:
            return 16u;
        default:
            return 0u;
    }
}

VkFormat vertex_format(const AttrFormat fmt) {
    switch (fmt) {
        case AttrFormat::X32_SFloat:
            return VK_FORMAT_R32_SFLOAT;
        case AttrFormat::XY32_SFloat:
            return VK_FORMAT_R32G32_SFLOAT;
        case AttrFormat::XYZ32_SFloat:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case AttrFormat::XYZW32_SFloat:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:
            return VK_FORMAT_UNDEFINED;
    }
}

VkPrimitiveTopology primitive_topology(const Topology topology) {
    switch (topology) {
        case Topology::TriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case Topology::LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        default:
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    }
}

} /* translate */
