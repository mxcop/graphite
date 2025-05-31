#pragma once

#include "platform/platform.hh"

#include "gpu_adapter.hh"
#include "result.hh"

/* Include platform-specific Impl struct */
struct ImplRenderGraph;
#include PLATFORM_H(render_graph)

/**
 * Render Graph.  
 * Used to queue and dispatch a graph of render passes.
 */
class RenderGraph : ImplRenderGraph {

public: /* Platform-agnostic functions */

public: /* Platform-specific functions */
    /* Initialize the Render Graph. */
    Result<void> init(GPUAdapter& gpu);

    /* Destroy the Render Graph, free all its resources. */
    Result<void> destroy();
};
