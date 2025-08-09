#include "translate_vk.hh"

namespace translate {

/* Convert the platform-agnostic stage flags to Vulkan shader stage flags. */
VkShaderStageFlags stage_flags(DependencyStages stages) {
    VkShaderStageFlags flags = 0x00;
    if (has_flag(stages, DependencyStages::Compute)) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    return flags;
}

} /* translate */
