#pragma once

#include "platform/platform.hh"

#include "gpu_adapter.hh"
#include "utils/result.hh"
#include "resources/stock.hh"

/* Slots are defined per platform */
PLATFORM_STRUCT struct TargetDesc;
PLATFORM_STRUCT struct RenderTargetSlot;

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

    /* Initialize the VRAM bank. */
    Result<void> init(GPUAdapter& gpu);

    /* Get a render target slot by handle. */
    RenderTargetSlot& get_render_target(OpaqueHandle render_target);
    /* Get a render target slot by handle. */
    const RenderTargetSlot& get_render_target(OpaqueHandle render_target) const;

public:
    /* Create a new render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC Result<RenderTarget> create_render_target(const TargetDesc& target, u32 width = 1440u, u32 height = 810u) = 0;

    /* Destroy a render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC void destroy_render_target(RenderTarget& render_target) = 0;

    /* Destroy the VRAM bank, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy() = 0;

    /* To access the init function. */
    friend class AgnGPUAdapter;
};

#include PLATFORM_INCLUDE(vram_bank)
