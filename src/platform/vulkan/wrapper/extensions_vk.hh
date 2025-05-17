#pragma once

#include <volk.h> /* Vulkan API */

#include "graphite/result.hh"

/* Required Vulkan device extensions */
const char* const device_ext[] = { 
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
    /* Reading storage images without format */
    VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME,
    /* Dynamic rendering */
    VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
    VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    /* Synchronization 2 */
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    /* Push descriptors & descriptor indexing */
    VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    /* Draw parameters, e.g. SV_InstanceID */
    VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
    /* Debugging info for shaders */
    VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
#if defined(_WIN32) || defined(_WIN64)
    /* OBS extension (windows only) */
    "VK_KHR_external_memory_win32"
#endif
};
const uint32_t device_ext_count = sizeof(device_ext) / sizeof(char*);

/* Query whether the driver supports debug utils. (needed for the debug messenger) */
bool query_debug_support(const char* layer_name);

/* Query whether the instance supports validation layers. */
bool query_validation_support(const char* layer_name);

/* Query whether a physical device supports all required extensions. */
Result<void> query_extension_support(const VkPhysicalDevice device, const char* const* extensions, const uint32_t count);
