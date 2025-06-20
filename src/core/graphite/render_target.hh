#pragma once

#include "platform/platform.hh"

#include "gpu_adapter.hh"
#include "utils/result.hh"
#include "utils/types.hh"
#include "resources/handle.hh"

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
    /* Resource handle */
    BindHandle handle {};

    /* Target extent */
    u32 width = 0u, height = 0u;

    /* ===== Platform-agnostic ===== */
public: 
    /* Re-size the Render Target. */
    Result<void> resize(u32 new_width, u32 new_height);

    /* ===== Platform-specific ===== */
public: 
    /* Destroy the Render Target, free all its resources. */
    Result<void> destroy(GPUAdapter& gpu);

private:
    /* Initialize the Render Target. */
    Result<void> init(GPUAdapter& gpu, TargetDesc& target, u32 def_width = 1440u, u32 def_height = 810u);

    /* Re-build the Render Target. */
    Result<void> rebuild();

    /* To access the `handle` variable. */
    friend class Dependency;
    /* To access the init function. */
    friend class VRAMBank;
};
