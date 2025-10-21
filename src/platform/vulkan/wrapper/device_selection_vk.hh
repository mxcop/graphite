#pragma once

#include <string>

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "graphite/utils/result.hh"
#include "graphite/utils/types.hh"

/* Vulkan physical device type ranking */
const i32 device_ranking[] = { 
    0, /* VK_PHYSICAL_DEVICE_TYPE_OTHER */ 
    2, /* VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU */ 
    4, /* VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU */ 
    3, /* VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU */ 
    1, /* VK_PHYSICAL_DEVICE_TYPE_CPU */ 
};

/* Select the most suitable, available, physical device. */
Result<VkPhysicalDevice> select_physical_device(VkInstance instance, const char* const* extensions, const u32 count);

/* Get the name of a physical device. */
std::string get_physical_device_name(VkPhysicalDevice physical_device);
