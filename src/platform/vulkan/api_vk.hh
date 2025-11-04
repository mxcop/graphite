#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define VK_USE_PLATFORM_WIN32_KHR
#else
#error "TODO: provide volk platform define for this platform!"
#endif

#include <volk.h> /* Vulkan API */

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h> /* VMA */

constexpr uint32_t VK_API_VERSION = VK_API_VERSION_1_2;

/* Divide two numbers, rounding up. */
template <typename T>
T div_up(const T x, const T y) { return (x + y - 1) / y; }
