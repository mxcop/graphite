#include "translate_vk.hh"

namespace translate {

/* Convert the platform-agnostic stage flags to Vulkan shader stage flags. */
VkShaderStageFlags stage_flags(DependencyStages stages) {
    VkShaderStageFlags flags = 0x00;
    if (has_flag(stages, DependencyStages::Compute)) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    return flags;
}

/* Convert the platform-agnostic dependency flags to a desired image layout. */
VkImageLayout desired_image_layout(TextureUsage usage, DependencyFlags flags) {
    if (has_flag(flags, DependencyFlags::Readonly)) {
        /* If texture is used as readonly resource, Sampled gets priority. */
        if (has_flag(usage, TextureUsage::Sampled)) return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        if (has_flag(usage, TextureUsage::Storage)) return VK_IMAGE_LAYOUT_GENERAL;
    } else {
        /* If texture is used as write resource, only Storage will work. */
        if (has_flag(usage, TextureUsage::Storage)) return VK_IMAGE_LAYOUT_GENERAL;
    }
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

/* Convert the platform-agnostic buffer usage to Vulkan buffer descriptor type. */
VkDescriptorType image_descriptor_type(TextureUsage usage, DependencyFlags flags) {
    if (has_flag(flags, DependencyFlags::Readonly)) {
        /* If texture is used as readonly resource, Sampled gets priority. */
        if (has_flag(usage, TextureUsage::Sampled)) return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        if (has_flag(usage, TextureUsage::Storage)) return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    } else {
        /* If texture is used as write resource, only Storage will work. */
        if (has_flag(usage, TextureUsage::Storage)) return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    }
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
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
VkBufferUsageFlags buffer_usage(BufferUsage usage) {
    VkBufferUsageFlags flags = 0x00;
    if (has_flag(usage, BufferUsage::TransferDst)) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (has_flag(usage, BufferUsage::TransferSrc)) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (has_flag(usage, BufferUsage::Constant)) flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (has_flag(usage, BufferUsage::Storage)) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    //if (has_flag(usage, BufferUsage::eVertex)) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    //if (has_flag(usage, BufferUsage::eIndex)) flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    //if (has_flag(usage, BufferUsage::eIndirect)) flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    return flags;
}

/* Convert the platform-agnostic buffer usage to Vulkan buffer descriptor type. */
VkDescriptorType buffer_descriptor_type(BufferUsage usage) {
    if (has_flag(usage, BufferUsage::Constant)) return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    if (has_flag(usage, BufferUsage::Storage)) return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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
    if (has_flag(usage, TextureUsage::Sampled)) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (has_flag(usage, TextureUsage::TransferDst)) flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (has_flag(usage, TextureUsage::TransferSrc)) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    return flags;
}

/* Convert the platform-agnostic filter to Vulkan sampler filter. */
VkFilter sampler_filter(Filter filter) {
    return filter == Filter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
}

/* Convert the platform-agnostic address mode to Vulkan sampler address mode. */
VkSamplerAddressMode sampler_address_mode(AddressMode mode) {
    switch (mode) {
        case AddressMode::Repeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case AddressMode::MirrorRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case AddressMode::ClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case AddressMode::ClampToBorder:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        default:
            return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
    }
}

/* Convert the platform-agnostic border color to Vulkan sampler border color. */
VkBorderColor sampler_border_color(BorderColor color) {
    switch (color) {
        case BorderColor::RGB0A0_Float:
            return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        case BorderColor::RGB0A0_Int:
            return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
        case BorderColor::RGB0A1_Float:
            return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        case BorderColor::RGB0A1_Int:
            return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        case BorderColor::RGB1A1_Float:
            return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        case BorderColor::RGB1A1_Int:
            return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        default:
            return VK_BORDER_COLOR_MAX_ENUM;
    }
}

} /* translate */
