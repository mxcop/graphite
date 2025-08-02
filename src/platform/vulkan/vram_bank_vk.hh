#pragma once

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
    VkSurfaceKHR surface {};
};

struct ImplVRAMBank {};

/* Interface header */
#include "graphite/vram_bank.hh"
