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
    /* VMA Resources */
    VmaAllocator vma_allocator {};

    /* Initialize the VRAM bank. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu);
public:
    
    /* Create a new render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC Result<RenderTarget> create_render_target(const TargetDesc& target, u32 width = 1440u, u32 height = 810u);
    
    /* Resize a render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC Result<void> resize_render_target(RenderTarget& render_target, u32 width, u32 height);

    /* Destroy a render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC void destroy_render_target(RenderTarget& render_target);

    /**
     * @brief Allocate a new buffer resource.
     * @param count If "stride" is 0 this represents the number of bytes in the buffer (for Constant buffers),
     * otherwise it is the number of elements in the buffer.
     * @param stride The size in bytes of an element in the buffer, leave 0 for Constant buffers.
     */
    PLATFORM_SPECIFIC Result<Buffer> create_buffer(const BufferUsage usage, const u64 count, const u64 stride = 0);

    /* Destroy a buffer resource. */
    PLATFORM_SPECIFIC void destroy_buffer(Buffer& buffer);

    /* Destroy the VRAM bank, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy();

    /* To access resource getters. "./wrapper/descriptor_vk.cc" */
    friend Result<void> node_push_descriptors(const RenderGraph& rg, const Pipeline& pipeline, const Node& node);
    friend Result<void> wave_sync_descriptors(const RenderGraph& rg, u32 start, u32 end);
    /* To access resource getters. */
    friend class RenderGraph;
    friend class AgnGPUAdapter;
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
    VkExtent2D extent {};
    u32 current_image = 0u;
    
    /* Swapchain image resources */
    VkImageLayout* old_layouts {};
    VkSemaphore* semaphores {};
    VkImageView* views {};
    VkImage* images {};

    /* Current image getters */
    inline VkImage& image() { return images[current_image]; };
    inline VkImageView& view() { return views[current_image]; };
    inline VkSemaphore& semaphore() { return semaphores[current_image]; };
    inline VkImageLayout& old_layout() { return old_layouts[current_image]; };
};

/* Buffer resource slot. */
struct BufferSlot {
    VkBuffer buffer {};
    VmaAllocation alloc {};
    BufferUsage usage {};
    u64 size = 0u;
};
