#pragma once

#include <vector>

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

    /* Vulkan bindless resources */
    VkDescriptorPool bindless_pool {};
    VkDescriptorSetLayout bindless_layout {};
    VkDescriptorSet bindless_set {};

    /* Upload Resources */
    VkCommandPool upload_cmd_pool {};
    VkCommandBuffer upload_cmd {};
    VkFence upload_fence {};

    /* Initialize the VRAM bank. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu);

    /* Destroy a render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC void destroy_render_target(RenderTarget& render_target);
    /* Destroy a buffer resource. */
    PLATFORM_SPECIFIC void destroy_buffer(Buffer& buffer);
    /* Destroy a texture resource. */
    PLATFORM_SPECIFIC void destroy_texture(Texture& texture);
    /* Destroy a image resource. */
    PLATFORM_SPECIFIC void destroy_image(Image& image);
    /* Destroy a sampler resource. */
    PLATFORM_SPECIFIC void destroy_sampler(Sampler& sampler);

    /* Begin recording immediate commands. */
    bool begin_upload();
    /* End recording immediate commands, submit, and wait for commands to finish. */
    bool end_upload();

public:
    /* Create a new render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC Result<RenderTarget> create_render_target(const TargetDesc& target, bool vsync = true, u32 width = 1440u, u32 height = 810u);
    /**
     * @brief Create a new buffer resource.
     * @param count If "stride" is 0 this represents the number of bytes in the buffer,
     * otherwise it is the number of elements in the buffer.
     * @param stride The size in bytes of an element in the buffer, leave 0 for Constant buffers.
     */
    PLATFORM_SPECIFIC Result<Buffer> create_buffer(BufferUsage usage, u64 count, u64 stride = 0);
    /* Create a new texture resource. */
    PLATFORM_SPECIFIC Result<Texture> create_texture(TextureUsage usage, TextureFormat fmt, Size3D size, TextureMeta meta = TextureMeta());
    /* Create a new image resource. */
    PLATFORM_SPECIFIC Result<Image> create_image(Texture texture, u32 mip = 0u, u32 layer = 0u);
    /* Create a new sampler resource. */
    PLATFORM_SPECIFIC Result<Sampler> create_sampler(Filter filter = Filter::Linear, AddressMode mode = AddressMode::Repeat, BorderColor border = BorderColor::RGB0A0_Float);

    /* Resize a render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC Result<void> resize_render_target(RenderTarget& render_target, u32 width, u32 height);
    /* Resize a texture resource. */
    PLATFORM_SPECIFIC Result<void> resize_texture(Texture& texture, Size3D size);

    /* Upload data to a GPU buffer resource. */
    PLATFORM_SPECIFIC Result<void> upload_buffer(Buffer& buffer, const void* data, u64 dst_offset, u64 size);
    /* Upload data to a GPU texture resource. */
    PLATFORM_SPECIFIC Result<void> upload_texture(Texture& texture, const void* data, const u64 size);

    /* Get the texture which an image was created from. */
    PLATFORM_SPECIFIC Texture get_texture(Image image);

    /* De-initialize the VRAM bank, free all its resources. */
    PLATFORM_SPECIFIC Result<void> deinit();

    /* To access resource getters. "./wrapper/descriptor_vk.cc" */
    friend Result<void> node_push_descriptors(const RenderGraph& rg, const Pipeline& pipeline, const Node& node);
    friend Result<void> wave_sync_descriptors(const RenderGraph& rg, u32 start, u32 end);
    /* To access resource getters. */
    friend class RenderGraph;
    friend class AgnGPUAdapter;
    friend class ImGUI;
    friend class PipelineCache;
};

/* Render target resource slot. */
struct RenderTargetSlot {
    /* Surface resources */
    VkSurfaceKHR surface {};
    u32 image_count = 0u;
    VkFormat format {};
    VkColorSpaceKHR color_space {};
    VkPresentModeKHR present_mode {};

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
    /* Allocation */
    VmaAllocation alloc {};

    /* Resource */
    VkBuffer buffer {};

    /* Metadata */
    BufferUsage usage {};
    u64 size = 0u;
};

/* Texture resource slot. */
struct TextureSlot {
    /* Allocation */
    VmaAllocation alloc {};

    /* Resource */
    VkImage image {};
    VkImageLayout layout {};

    /* Metadata */
    Size3D size {};
    TextureUsage usage {};
    TextureFormat format {};
    TextureMeta meta {};

    /* List of Images created from this Texture */
    std::vector<Image> images {};
};

/* Image resource slot. */
struct ImageSlot {
    /* Resource reference */
    Texture texture {};

    /* Image view */
    VkImageView view {};
    VkImageSubresourceRange sub_range {};
};

/* Sampler resource slot. */
struct SamplerSlot {
    VkSampler sampler {};
};
