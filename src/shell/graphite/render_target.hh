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

public: /* Platform-agnostic functions */

public: /* Platform-specific functions */
    /* Initialize the Render Target. */
    Result<void> init(void* surface);

    /* Destroy the Render Target, free all its resources. */
    Result<void> destroy();
};
