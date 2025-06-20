#pragma once

#include "platform/platform.hh"

#include "gpu_adapter.hh"
#include "utils/result.hh"
#include "resources/stock.hh"

struct TargetDesc;

/* Include platform-specific Impl struct */
struct ImplVRAMBank;
#include PLATFORM_H(vram_bank)

/**
 * Video Memory Bank / Video Resource Manager.  
 * Used to create and manage GPU resources.
 */
class VRAMBank : ImplVRAMBank {
    GPUAdapter* gpu = nullptr;

    /* Resources */
    Stock<RenderTarget, ResourceType::RenderTarget> render_targets {};

    /* ===== Platform-agnostic ===== */
public: 
    /* Initialize the VRAM bank. */
    Result<void> init(GPUAdapter& gpu);

    /* Create a new render target resource. (aka, swapchain) */
    Result<RenderTarget*> create_render_target(TargetDesc& target, u32 def_width = 1440u, u32 def_height = 810u);

    /* ===== Platform-specific ===== */
public:
    /* Destroy the VRAM bank, free all its resources. */
    Result<void> destroy();
};
