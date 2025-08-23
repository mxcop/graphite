#pragma once

#include "platform/platform.hh"

#include "gpu_adapter.hh"
#include "utils/result.hh"
#include "resources/stock.hh"

/* Slots are defined per platform */
PLATFORM_SPECIFIC struct TargetDesc;
PLATFORM_SPECIFIC struct RenderTargetSlot;

/* Include platform-specific Impl struct */
PLATFORM_SPECIFIC struct ImplVRAMBank;
#include PLATFORM_H(vram_bank)

/**
 * Video Memory Bank / Video Resource Manager.  
 * Used to create and manage GPU resources.
 */
class VRAMBank : public ImplVRAMBank {
    GPUAdapter* gpu = nullptr;

    /* Resources */
    Stock<RenderTargetSlot, RenderTarget, ResourceType::RenderTarget> render_targets {};

    /* Initialize the VRAM bank. */
    Result<void> init(GPUAdapter& gpu);

    /* Get a render target slot by handle. */
    RenderTargetSlot& get_render_target(RenderTarget render_target);

public:
    /* Create a new render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC Result<RenderTarget> create_render_target(const TargetDesc& target, u32 width = 1440u, u32 height = 810u);

    /* Destroy a render target resource. (aka, swapchain) */
    PLATFORM_SPECIFIC void destroy_render_target(RenderTarget& render_target);

    /* Destroy the VRAM bank, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy();

    /* To access the init function. */
    friend class GPUAdapter;
    /* To access the getter functions. */
    friend class RenderGraph;
};
