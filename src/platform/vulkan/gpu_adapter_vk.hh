#pragma once

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
