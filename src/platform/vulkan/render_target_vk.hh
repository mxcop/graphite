#pragma once

#include "vulkan/api_vk.hh" /* Vulkan API */

struct ImplRenderTarget {
    VkSurfaceKHR surface {};
};

/* Interface header */
#include "graphite/render_target.hh"
