#pragma once

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "wrapper/queue_selection_vk.hh"

struct ImplGPUAdapter {
    VkInstance instance {};
    VkPhysicalDevice physical_device {};
    VkQueueFamilies queue_families {};
    VkDevice logical_device {};

    /* Vulkan debug / validation */
    bool validation = false;
    VkDebugUtilsMessengerEXT debug_messenger {};
};

/* Interface header */
#include "graphite/gpu_adapter.hh"
