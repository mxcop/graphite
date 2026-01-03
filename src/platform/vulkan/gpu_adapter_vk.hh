#pragma once

/* Interface header */
#include "graphite/gpu_adapter.hh"

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "wrapper/queue_selection_vk.hh"

#ifdef GRAPHITE_TRACY
#include <tracy/TracyVulkan.hpp>
#endif

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

#ifdef GRAPHITE_TRACY
    /* Tracy */
    TracyVkCtx tracy_ctx {};
#endif

public:
    /* Initialize the GPU adapter. */
    PLATFORM_SPECIFIC Result<void> init(bool debug_mode = false);

    /* Hook up the Tracy profiler. */
    PLATFORM_SPECIFIC void hook_tracy();

    /* De-initialize the GPU adapter, free all its resources. */
    PLATFORM_SPECIFIC Result<void> deinit();

    /* To access the hidden graphics resources. */
    friend class PipelineCache;
    friend class VRAMBank;
    friend class RenderGraph;
    friend class ImGUI;

    /* To access VRAM Bank and logical device */
    friend Result<VkDescriptorSetLayout> node_descriptor_layout(GPUAdapter& gpu, const Node& node);
};
