#include "translate_vk.hh"

namespace translate {

/* Convert the platform-agnostic stage flags to Vulkan shader stage flags. */
VkShaderStageFlags stage_flags(DependencyStages stages) {
    VkShaderStageFlags flags = 0x00;
    if (has_flag(stages, DependencyStages::Compute)) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    return flags;
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

} /* translate */
