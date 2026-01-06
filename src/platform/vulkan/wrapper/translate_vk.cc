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
VkImageLayout desired_image_layout(TextureUsage usage, DependencyFlags flags) {
    if (has_flag(flags, DependencyFlags::Readonly)) {
        /* If texture is used as readonly resource, Sampled gets priority. */
        if (has_flag(usage, TextureUsage::Sampled)) return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        if (has_flag(usage, TextureUsage::Storage)) return VK_IMAGE_LAYOUT_GENERAL;
    } else {
        /* If texture is used as write resource, only Storage will work. */
        if (has_flag(usage, TextureUsage::Storage)) return VK_IMAGE_LAYOUT_GENERAL;
        if (has_flag(flags, DependencyFlags::Attachment)) return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
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
        case NodeType::Raster:
            return VK_PIPELINE_BIND_POINT_GRAPHICS;
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
    if (has_flag(usage, BufferUsage::Vertex)) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (has_flag(usage, BufferUsage::Indirect)) flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
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
        case TextureFormat::RG32Uint:
            return VK_FORMAT_R32G32_UINT;
        case TextureFormat::RGBA16Sfloat:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureFormat::RG11B10Ufloat:
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
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
    if (has_flag(usage, TextureUsage::ColorAttachment)) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    return flags;
}

/* Convert the platform-agnostic filter to Vulkan sampler filter. */
VkFilter sampler_filter(Filter filter) { return filter == Filter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR; }

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

/* Get the number of bytes per vertex attribute for a given vertex attribute format. */
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

/* Convert the platform-agnostic vertex attribute format to a vertex format. */
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

/* Convert the platform-agnostic primitive topology. */
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

/* Convert the platform-agnostic load operation. */
VkAttachmentLoadOp load_operation(const LoadOp op) {
    switch (op) {
        case LoadOp::Load:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case LoadOp::Clear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        default:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
}

/* Convert the platform-agnostic vertex input rate. */
VkVertexInputRate vertex_input_rate(const VertexInputRate rate) {
    switch (rate) {
        case VertexInputRate::Vertex:
            return VK_VERTEX_INPUT_RATE_VERTEX;
        case VertexInputRate::Instance:
            return VK_VERTEX_INPUT_RATE_INSTANCE;
        default:
            return VK_VERTEX_INPUT_RATE_MAX_ENUM;
    }
}

}  // namespace translate
