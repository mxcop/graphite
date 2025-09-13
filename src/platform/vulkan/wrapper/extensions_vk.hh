#pragma once

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "graphite/utils/result.hh"
#include "graphite/utils/types.hh"

/* Required Vulkan device extensions */
const char* const device_ext[] = { 
    /* Swapchain extension */
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    /* Push descriptors */
    VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
    /* For SPIR-V "StorageImageReadWithoutFormat" and "StorageImageWriteWithoutFormat" */
    VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME
};
const u32 device_ext_count = sizeof(device_ext) / sizeof(char*);

/* Query whether the driver supports debug utils. (needed for the debug messenger) */
bool query_debug_support(const char* layer_name);

/* Query whether the driver supports our required instance extensions. */
Result<void> query_instance_support(const char* const* extensions, const u32 count);

/* Query whether the instance supports validation layers. */
bool query_validation_support(const char* layer_name);

/* Query whether a physical device supports all required extensions. */
Result<void> query_extension_support(const VkPhysicalDevice device, const char* const* extensions, const u32 count);
