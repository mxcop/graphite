#pragma once

#include "platform/platform.hh"

#include "gpu_adapter.hh"
#include "utils/result.hh"

#include "resources/buffer.hh"
#include "resources/texture.hh"
#include "resources/stock.hh"

/* Slots are defined per platform */
PLATFORM_STRUCT struct TargetDesc;
PLATFORM_STRUCT struct RenderTargetSlot;
PLATFORM_STRUCT struct BufferSlot;
PLATFORM_STRUCT struct TextureSlot;
PLATFORM_STRUCT struct ImageSlot;
PLATFORM_STRUCT struct SamplerSlot;

class GPUAdapter;

/**
 * @warning Never use this class directly!
 * This is an interface for the platform-specific class.
 */
class AgnVRAMBank {
protected:
    GPUAdapter* gpu = nullptr;

    /* Resources */
    Stock<RenderTargetSlot, RenderTarget, ResourceType::RenderTarget> render_targets {};
    Stock<BufferSlot, Buffer, ResourceType::Buffer> buffers {};
    Stock<TextureSlot, Texture, ResourceType::Texture> textures {};
    Stock<ImageSlot, Image, ResourceType::Image> images {};

    /* Initialize the VRAM bank. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu) = 0;

    /* Get a render target slot by handle. */
    RenderTargetSlot& get_render_target(BindHandle render_target);
    /* Get a render target slot by handle. */
    const RenderTargetSlot& get_render_target(BindHandle render_target) const;

    /* Get a buffer slot by handle. */
    BufferSlot& get_buffer(BindHandle buffer);
    /* Get a buffer slot by handle. */
    const BufferSlot& get_buffer(BindHandle buffer) const;

    /* Get a texture slot by handle. */
    TextureSlot& get_texture(OpaqueHandle texture);
    /* Get a texture slot by handle. */
    const TextureSlot& get_texture(OpaqueHandle texture) const;

    /* Get a image slot by handle. */
    ImageSlot& get_image(BindHandle image);
    /* Get a image slot by handle. */
    const ImageSlot& get_image(BindHandle image) const;

public:
    /* Create a new render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC Result<RenderTarget> create_render_target(const TargetDesc& target, u32 width = 1440u, u32 height = 810u) = 0;
    /**
     * @brief Create a new buffer resource.
     * @param count If "stride" is 0 this represents the number of bytes in the buffer (for Constant buffers),
     * otherwise it is the number of elements in the buffer.
     * @param stride The size in bytes of an element in the buffer, leave 0 for Constant buffers.
     */
    PLATFORM_SPECIFIC Result<Buffer> create_buffer(BufferUsage usage, u64 count, u64 stride = 0) = 0;
    /* Create a new texture resource. */
    PLATFORM_SPECIFIC Result<Texture> create_texture(TextureUsage usage, TextureFormat fmt, Size3D size, TextureMeta meta = TextureMeta()) = 0;
    /* Create a new image resource. */
    PLATFORM_SPECIFIC Result<Image> create_image(Texture texture, u32 mip = 0u, u32 layer = 0u) = 0;

    /* Resize a render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC Result<void> resize_render_target(RenderTarget& render_target, u32 width, u32 height) = 0;
    /* Upload data to a GPU buffer resource. */
    PLATFORM_SPECIFIC Result<void> upload_buffer(Buffer& buffer, const void* data, u64 dst_offset, u64 size) = 0;

    /* Destroy a render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC void destroy_render_target(RenderTarget& render_target) = 0;
    /* Destroy a buffer resource. */
    PLATFORM_SPECIFIC void destroy_buffer(Buffer& buffer) = 0;
    /* Destroy a texture resource. */
    PLATFORM_SPECIFIC void destroy_texture(Texture& texture) = 0;
    /* Destroy a image resource. */
    PLATFORM_SPECIFIC void destroy_image(Image& image) = 0;

    /* Destroy the VRAM bank, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy() = 0;

    /* To access the init function. */
    friend class AgnGPUAdapter;

    /* To access the get_buffer() function. */
    friend Result<VkDescriptorSetLayout> node_descriptor_layout(GPUAdapter& gpu, const Node& node);
};

#include PLATFORM_INCLUDE(vram_bank)
