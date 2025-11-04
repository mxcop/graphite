#include "translate_vk.hh"

namespace translate {

/* Convert the platform-agnostic stage flags to Vulkan shader stage flags. */
VkShaderStageFlags stage_flags(DependencyStages stages) {
    VkShaderStageFlags flags = 0x00;
    if (has_flag(stages, DependencyStages::Compute)) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
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
        default:
            return VK_PIPELINE_BIND_POINT_MAX_ENUM;
    }
}

/* Convert the platform-agnostic buffer usage to buffer usage flags. */
VkBufferUsageFlags buffer_usage(const BufferUsage usage) {
    VkBufferUsageFlags flags = 0x00;
    if (has_flag(usage, BufferUsage::TransferDst)) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (has_flag(usage, BufferUsage::TransferSrc)) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (has_flag(usage, BufferUsage::Constant)) flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    //if (has_flag(usage, BufferUsage::eStorage)) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    //if (has_flag(usage, BufferUsage::eVertex)) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    //if (has_flag(usage, BufferUsage::eIndex)) flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    //if (has_flag(usage, BufferUsage::eIndirect)) flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    return flags;
}

/* Convert the platform-agnostic buffer usage to Vulkan buffer descriptor type. */
VkDescriptorType buffer_descriptor_type(const BufferUsage usage) {
    if (has_flag(usage, BufferUsage::Constant)) return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //if (has_flag(usage, BufferUsage::Storage)) return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

/* Convert the platform-agnostic texture format to Vulkan texture format. */
VkFormat texture_format(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGBA8Unorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
        default:
            return VK_FORMAT_UNDEFINED;
    }
}

/* Convert the platform-agnostic texture usage to texture usage flags. */
VkImageUsageFlags texture_usage(TextureUsage usage) {
    VkImageUsageFlags flags = 0x00;
    if (has_flag(usage, TextureUsage::Storage)) flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (has_flag(usage, TextureUsage::TransferDst)) flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (has_flag(usage, TextureUsage::TransferSrc)) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    return flags;
}

} /* translate */
