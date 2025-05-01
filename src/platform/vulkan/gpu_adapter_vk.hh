#pragma once

#include <volk.h> /* Vulkan API */

struct ImplGPUAdapter {
    VkInstance instance;
};

/* Interface header */
#include "graphite/gpu_adapter.hh"
