#pragma once

#include "platform/platform.hh"

#include "gpu_adapter.hh"
#include "utils/result.hh"

#include "resources/buffer.hh"
#include "resources/texture.hh"
#include "resources/sampler.hh"
#include "resources/stock.hh"

/* Slots are defined per platform */
PLATFORM_STRUCT struct TargetDesc;
PLATFORM_STRUCT struct RenderTargetSlot;
PLATFORM_STRUCT struct BufferSlot;
PLATFORM_STRUCT struct TextureSlot;
PLATFORM_STRUCT struct ImageSlot;
PLATFORM_STRUCT struct SamplerSlot;

class GPUAdapter;

/* Constexprs */
constexpr u32 BINDLESS_TEXTURE_SLOT = 0u;
constexpr u32 BINDLESS_BUFFER_SLOT = 1u;

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
    Stock<SamplerSlot, Sampler, ResourceType::Sampler> samplers {};

    /* Initialize the VRAM bank. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu) = 0;

    /* Destroy a render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC void destroy_render_target(RenderTarget& render_target) = 0;
    /* Destroy a buffer resource. */
    PLATFORM_SPECIFIC void destroy_buffer(Buffer& buffer) = 0;
    /* Destroy a texture resource. */
    PLATFORM_SPECIFIC void destroy_texture(Texture& texture) = 0;
    /* Destroy a image resource. */
    PLATFORM_SPECIFIC void destroy_image(Image& image) = 0;
    /* Destroy a sampler resource. */
    PLATFORM_SPECIFIC void destroy_sampler(Sampler& sampler) = 0;

    /* Add a reference to a resource. (for reference counting) */
    void add_reference(OpaqueHandle resource);
    /* Remove a reference to a resource. (will destroy the resource if no references are left) */
    void remove_reference(OpaqueHandle resource);

public:
    /* Create a new render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC Result<RenderTarget> create_render_target(const TargetDesc& target, bool vsync = true, u32 width = 1440u, u32 height = 810u) = 0;
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
    /* Create a new sampler resource. */
    PLATFORM_SPECIFIC Result<Sampler> create_sampler(Filter filter = Filter::Linear, AddressMode mode = AddressMode::Repeat, BorderColor border = BorderColor::RGB0A0_Float) = 0;

    /* Resize a render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC Result<void> resize_render_target(RenderTarget& render_target, u32 width, u32 height) = 0;
    /* Upload data to a GPU buffer resource. */
    PLATFORM_SPECIFIC Result<void> upload_buffer(Buffer& buffer, const void* data, u64 dst_offset, u64 size) = 0;
    /* Upload data to a GPU texture resource. */
    PLATFORM_SPECIFIC Result<void> upload_texture(Texture& texture, const void* data, const u64 size) = 0;

    /* Get the texture which an image was created from. */
    PLATFORM_SPECIFIC Texture get_texture(Image image) = 0;

    /* Destroy a resource. */
    void destroy(OpaqueHandle& resource);

    /* De-initialize the VRAM bank, free all its resources. */
    PLATFORM_SPECIFIC Result<void> deinit() = 0;

    /* To access the init function. */
    friend class AgnGPUAdapter;
    friend class AgnRenderGraph;

    /* To access the get_buffer() function. */
    friend Result<VkDescriptorSetLayout> node_descriptor_layout(GPUAdapter& gpu, const Node& node);
};

#include PLATFORM_INCLUDE(vram_bank)
