#pragma once

/* Interface header */
#include "graphite/gpu_adapter.hh"

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "wrapper/queue_selection_vk.hh"

/**
 * Graphics Processing Unit Adapter.  
 * Used to find and initialize a GPU.
 */
class GPUAdapter : public AgnGPUAdapter {
    /* Vulkan gpu state */
    VkInstance instance {};
    VkPhysicalDevice physical_device {};
    VkQueueFamilies queue_families {};
    VkDevice logical_device {};
    VkQueues queues {};

    /* Vulkan debug / validation */
    bool validation = false;
    VkDebugUtilsMessengerEXT debug_messenger {};

public:
    /* Initialize the GPU adapter. */
    PLATFORM_SPECIFIC Result<void> init(bool debug_mode = false);

    /* Destroy the GPU adapter, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy();

    /* To access the hidden graphics resources. */
    friend class PipelineCache;
    friend class VRAMBank;
    friend class RenderGraph;
    friend class ImGUI;
};
