#pragma once

#include "vulkan/api_vk.hh" /* Vulkan API */

struct TargetDesc {
#if defined(_WIN32) || defined(_WIN64)
    HWND window {};
#else
    #error "TODO: provide target descriptor for this platform!"
#endif
};

struct ImplRenderTarget {
    VkSurfaceKHR surface {};
};

/* Interface header */
#include "graphite/render_target.hh"
