#pragma once

/* Interface header */
#include "graphite/gpu_adapter.hh"

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "wrapper/queue_selection_vk.hh"

class Node;

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
    VkCommandPool cmd_pool {};

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

    /* To access VRAM Bank and logical device */
    friend Result<VkDescriptorSetLayout> node_descriptor_layout(GPUAdapter& gpu, const Node& node);
};
