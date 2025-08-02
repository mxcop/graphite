#pragma once

#include "graphite/utils/types.hh"
#include "vulkan/api_vk.hh" /* Vulkan API */

/* Render target descriptor, used during render target creation. */
struct TargetDesc {
#if defined(_WIN32) || defined(_WIN64)
    HWND window {};
#else
    #error "TODO: provide target descriptor for this platform!"
#endif
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
    VkExtent2D extent {};
};

struct ImplVRAMBank {};

/* Interface header */
#include "graphite/vram_bank.hh"
