#pragma once

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "graphite/utils/result.hh"

/* Collection of Vulkan queue families. */
struct VkQueueFamilies {
    /* The combined queue family is capable of everything. */
    uint32_t queue_combined = 0u;
    /* The (async) compute queue family is for running compute alongside graphics work. */
    uint32_t queue_compute = 0u;
    /* The transfer queue family is for asynchronously transferring data to the GPU. */
    uint32_t queue_transfer = 0u;
};

/* Collection of Vulkan queues. */
struct VkQueues {
    /* The combined queue family is capable of everything. */
    VkQueue queue_combined {};
    /* The (async) compute queue family is for running compute alongside graphics work. */
    VkQueue queue_compute {};
    /* The transfer queue family is for asynchronously transferring data to the GPU. */
    VkQueue queue_transfer {};
};

/* Select the combined, (async) compute, and transfer queue families. */
Result<VkQueueFamilies> select_queue_families(VkPhysicalDevice device);

/* Get the combined, (async) compute, and transfer queues. */
VkQueues get_queues(VkDevice device, const VkQueueFamilies& families);
