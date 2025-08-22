#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define VK_USE_PLATFORM_WIN32_KHR
#else
#error "TODO: provide volk platform define for this platform!"
#endif

#include <volk.h> /* Vulkan API */

/* Divide two numbers, rounding up. */
template <typename T>
T div_up(const T x, const T y) { return (x + y - 1) / y; }
