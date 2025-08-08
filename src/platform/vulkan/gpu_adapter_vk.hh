#pragma once

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "wrapper/queue_selection_vk.hh"

struct ImplGPUAdapter {
protected:
    /* Vulkan gpu state */
    VkInstance instance {};
    VkPhysicalDevice physical_device {};
    VkQueueFamilies queue_families {};
    VkDevice logical_device {};
    VkQueues queues {};

    /* Vulkan debug / validation */
    bool validation = false;
    VkDebugUtilsMessengerEXT debug_messenger {};

    /* To access the hidden graphics resources. */
    friend class PipelineCache;
};

/* Interface header */
#include "graphite/gpu_adapter.hh"
