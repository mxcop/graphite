#pragma once

#include <string>

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "graphite/utils/result.hh"

/* Vulkan physical device type ranking */
const int device_ranking[] = { 
    0, /* VK_PHYSICAL_DEVICE_TYPE_OTHER */ 
    2, /* VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU */ 
    4, /* VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU */ 
    3, /* VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU */ 
    1, /* VK_PHYSICAL_DEVICE_TYPE_CPU */ 
};

/* Select the most suitable, available, physical device. */
Result<VkPhysicalDevice> select_physical_device(VkInstance instance, const char* const* extensions, const uint32_t count);

/* Get the name of a physical device. */
std::string get_physical_device_name(VkPhysicalDevice physical_device);
