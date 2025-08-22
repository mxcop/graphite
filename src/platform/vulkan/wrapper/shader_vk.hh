#pragma once

#include <string>

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "graphite/utils/result.hh"

namespace shader {

/* Try to load a shader module from its path alias. */
Result<VkShaderModule> from_alias(const VkDevice device, const std::string_view path, const std::string_view alias);

} /* shader */
