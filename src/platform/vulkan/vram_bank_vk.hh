#pragma once

#include "vulkan/api_vk.hh" /* Vulkan API */

struct TargetDesc {
#if defined(_WIN32) || defined(_WIN64)
    HWND window {};
#else
    #error "TODO: provide target descriptor for this platform!"
#endif
};

struct RenderTargetSlot {
    int i = 0;
};

struct ImplVRAMBank {};

/* Interface header */
#include "graphite/vram_bank.hh"
