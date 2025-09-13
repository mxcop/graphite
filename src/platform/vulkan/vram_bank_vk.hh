#pragma once

/* Interface header */
#include "graphite/vram_bank.hh"

#include "graphite/utils/types.hh"
#include "vulkan/api_vk.hh" /* Vulkan API */

class Node;

struct GraphExecution;
struct Pipeline;

/* Render target descriptor, used during render target creation. */
struct TargetDesc {
#if defined(_WIN32) || defined(_WIN64)
    HWND window {};
#else
    #error "TODO: provide target descriptor for this platform!"
#endif
};

/**
 * Video Memory Bank / Video Resource Manager.  
 * Used to create and manage GPU resources.
 */
class VRAMBank : public AgnVRAMBank {
public:
    /* Create a new render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC Result<RenderTarget> create_render_target(const TargetDesc& target, u32 width = 1440u, u32 height = 810u);

    /* Destroy a render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC void destroy_render_target(RenderTarget& render_target);

    /* Destroy the VRAM bank, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy();

    /* To access resource getters. "./wrapper/descriptor_vk.cc" */
    friend Result<void> node_push_descriptors(const RenderGraph& rg, const Pipeline& pipeline, const Node& node);
    friend Result<void> wave_sync_descriptors(const RenderGraph& rg, u32 start, u32 end);
    /* To access resource getters. */
    friend class RenderGraph;
};

/* Render target resource slot. */
struct RenderTargetSlot {
    /* Surface resources */
    VkSurfaceKHR surface {};
    u32 image_count = 0u;
    VkFormat format {};
    VkColorSpaceKHR color_space {};

    /* Swapchain resources */
    VkSwapchainKHR swapchain {};
    VkImage* images {};
    VkImageView* views {};
    VkImageLayout* old_layouts {};
    VkSemaphore* acquired_semaphores {};
    VkExtent2D extent {};
    u32 current_image = 0u;
};
