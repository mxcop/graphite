#pragma once

#include "platform/platform.hh"

#include "gpu_adapter.hh"
#include "result.hh"

/* Include platform-specific Impl struct */
struct ImplVRAMBank;
#include PLATFORM_H(vram_bank)

/**
 * Video Memory Bank / Video Resource Manager.  
 * Used to create and manage GPU resources.
 */
class VRAMBank : ImplVRAMBank {

public: /* Platform-agnostic functions */

public: /* Platform-specific functions */
    /* Initialize the VRAM bank. */
    Result<void> init(GPUAdapter& gpu);

    /* Destroy the VRAM bank, free all its resources. */
    Result<void> destroy();
};
