#pragma once

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "graphite/result.hh"

/* Required Vulkan device extensions */
const char* const device_ext[] = { 
    /* Swapchain extension */
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const uint32_t device_ext_count = sizeof(device_ext) / sizeof(char*);

/* Query whether the driver supports debug utils. (needed for the debug messenger) */
bool query_debug_support(const char* layer_name);

/* Query whether the driver supports our required instance extensions. */
Result<void> query_instance_support(const char* const* extensions, const uint32_t count);

/* Query whether the instance supports validation layers. */
bool query_validation_support(const char* layer_name);

/* Query whether a physical device supports all required extensions. */
Result<void> query_extension_support(const VkPhysicalDevice device, const char* const* extensions, const uint32_t count);
