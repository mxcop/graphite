#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define VK_USE_PLATFORM_WIN32_KHR
#else
#error "TODO: provide volk platform define for this platform!"
#endif

#include <volk.h> /* Vulkan API */

struct ImplGPUAdapter {
    VkInstance instance {};
    VkPhysicalDevice physical_device {};

    /* Vulkan debug / validation */
    bool validation = false;
    VkDebugUtilsMessengerEXT debug_messenger {};
};

/* Interface header */
#include "graphite/gpu_adapter.hh"
