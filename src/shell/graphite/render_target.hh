#pragma once

#include "platform/platform.hh"

#include "result.hh"
#include "debug.hh"

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
    Result<void> init(void* target, uint32_t def_width = 1440u, uint32_t def_height = 810u);
    
    /* Destroy the Render Target, free all its resources. */
    Result<void> destroy();

private:
    /* Re-build the Render Target. */
    Result<void> rebuild();
};
