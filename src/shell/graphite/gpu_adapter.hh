#pragma once

#include "platform/platform.hh"

#include "result.hh"

/* Include platform-specific Impl struct */
struct ImplGPUAdapter;
#include PLATFORM_H(gpu_adapter)

class GPUAdapter : ImplGPUAdapter {
    int test = 21;

public: /* Platform-agnostic functions */
    int agn_function();

public: /* Platform-specific functions */
    Result<void> init(bool debug_mode = false);
};
