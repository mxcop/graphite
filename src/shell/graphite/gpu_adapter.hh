#pragma once

#include "platform/platform.hh"

struct ImplGPUAdapter;

#ifndef PLATFORM_IMPL
/* Include platform-specific Impl struct */
#include PLATFORM_H(gpu_adapter)
#endif

class GPUAdapter : ImplGPUAdapter {
    int test = 21;

public: /* Platform-agnostic functions */
    int agn_function();

public: /* Platform-specific functions */
    void spc_function();
};
