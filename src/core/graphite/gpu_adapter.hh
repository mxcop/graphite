#pragma once

#include "platform/platform.hh"

#include "utils/result.hh"
#include "utils/debug.hh"

class VRAMBank;

/* Include platform-specific Impl struct */
struct ImplGPUAdapter;
#include PLATFORM_H(gpu_adapter)

/* Debug logger data. */
struct DebugLogger {
    DebugLoggerFn logger = nullptr;
    DebugLevel log_level = DebugLevel::Warning;
    void* user_data = nullptr;
};

/**
 * Graphics Processing Unit Adapter.  
 * Used to find and initialize a GPU.
 */
class GPUAdapter : ImplGPUAdapter {
    /* Active debug logger. */
    DebugLogger logger {empty_logger, DebugLevel::Warning, nullptr};

    /* List of VRAM banks attached to this GPU adapter. */
    VRAMBank* vram_banks = nullptr;
    u32 vram_bank_count = 0u;

    /* ===== Platform-agnostic ===== */
public:
    /* Set the debug logger callback. */
    void set_logger(DebugLoggerFn callback = default_logger, DebugLevel level = DebugLevel::Warning, void* user_data = nullptr);

private:
    /* Log a message using the active debug logger. */
    void log(DebugSeverity severity, const char* msg);

    /* Initialize `count` number of VRAM banks for this GPU adapter. */
    Result<void> init_vram_banks(u32 count);

    /* ===== Platform-specific ===== */
public:
    /* Initialize the GPU adapter. */
    Result<void> init(u32 vram_bank_count = 1u, bool debug_mode = false);
    
    /* Get a VRAM bank from this GPU adapter. */
    Result<VRAMBank*> get_vram_bank(u32 bank_index = 0u);

    /* Destroy the GPU adapter, free all its resources. */
    Result<void> destroy();
};
