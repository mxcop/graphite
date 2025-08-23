#include "queue_selection_vk.hh"

/* Presentation support check for the current build platform */
#if defined(_WIN32) || defined(_WIN64)
#define vkGetPhysicalDevicePresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR
#else
#error "TODO: provide presentation support check for this platform!"
#endif

/* Select the combined, (async) compute, and transfer queue families. */
Result<VkQueueFamilies> select_queue_families(VkPhysicalDevice device) {
    /* Get the number of available queues */
    u32 queue_count = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, nullptr);

    /* Exit if no queues were found */
    if (queue_count == 0u) return Err("physical device has zero queue families available.");

    /* Get a list of available queue families */
    VkQueueFamilyProperties* queues = new VkQueueFamilyProperties[queue_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, queues);

    /* Look for suitable queues for different operations */
    u32 combined_queue = queue_count;
    u32 compute_queue = queue_count;
    u32 transfer_queue = queue_count;

    for (u32 i = 0u; i < queue_count; ++i) {
        /* Check what this queue family supports */
        const bool supports_present = (vkGetPhysicalDevicePresentationSupportKHR(device, i) == VK_TRUE);
        const bool supports_graphics = queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
        const bool supports_compute = queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
        const bool supports_transfer = queues[i].queueFlags & VK_QUEUE_TRANSFER_BIT;

        /* Select this queue as combined queue if we haven't already selected one */
        const bool missing_combined_queue = (combined_queue == queue_count);
        if (missing_combined_queue && supports_graphics && supports_compute && supports_present) {
            combined_queue = i;
        }

        /* Select this queue as dedicated compute queue */
        if (supports_compute && !supports_graphics) compute_queue = i;

        /* Select this queue as dedicated transfer queue */
        if (supports_transfer && !supports_graphics) transfer_queue = i;
    }

    /* If we didn't find a dedicated compute queue, use the combined queue instead */
    if (compute_queue == queue_count) compute_queue = combined_queue;

    /* If we didn't find a dedicated transfer queue, use the combined queue instead */
    if (transfer_queue == queue_count) transfer_queue = combined_queue;

    VkQueueFamilies out_queues {};
    out_queues.queue_combined = combined_queue;
    out_queues.queue_compute = compute_queue;
    out_queues.queue_transfer = transfer_queue;
    return Ok(out_queues);
}

/* Get the combined, (async) compute, and transfer queues. */
VkQueues get_queues(VkDevice device, const VkQueueFamilies& families) {
    VkQueues queues {};
    vkGetDeviceQueue(device, families.queue_combined, 0u, &queues.queue_combined);
    vkGetDeviceQueue(device, families.queue_compute, 0u, &queues.queue_compute);
    vkGetDeviceQueue(device, families.queue_transfer, 0u, &queues.queue_transfer);
    return queues;
}
