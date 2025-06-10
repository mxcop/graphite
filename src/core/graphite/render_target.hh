#pragma once

#include "platform/platform.hh"

#include "gpu_adapter.hh"
#include "utils/result.hh"

/* Render target descriptor. (differs per platform!) */
struct TargetDesc;

/* Include platform-specific Impl struct */
struct ImplRenderTarget;
#include PLATFORM_H(render_target)

/**
 * Swapchain / Render Target.  
 * Used to render to a window / display.
 */
class RenderTarget : ImplRenderTarget {
    uint32_t width = 0u, height = 0u;

    /* ===== Platform-agnostic ===== */
public: 
    /* Re-size the Render Target. */
    Result<void> resize(uint32_t new_width, uint32_t new_height);

    /* ===== Platform-specific ===== */
public: 
    /* Initialize the Render Target. */
    Result<void> init(GPUAdapter& gpu, TargetDesc& target, uint32_t def_width = 1440u, uint32_t def_height = 810u);
    
    /* Destroy the Render Target, free all its resources. */
    Result<void> destroy(GPUAdapter& gpu);

private:
    /* Re-build the Render Target. */
    Result<void> rebuild();
};
