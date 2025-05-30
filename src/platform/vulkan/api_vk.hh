#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define VK_USE_PLATFORM_WIN32_KHR
#else
#error "TODO: provide volk platform define for this platform!"
#endif

#include <volk.h> /* Vulkan API */
