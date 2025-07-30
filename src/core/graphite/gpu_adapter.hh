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

    /* VRAM bank for this GPU adapter. */
    VRAMBank* vram_bank = nullptr;

    /* ===== Platform-agnostic ===== */
public:
    /* Set the debug logger callback. */
    void set_logger(DebugLoggerFn callback = default_logger, DebugLevel level = DebugLevel::Warning, void* user_data = nullptr);

private:
    /* Log a message using the active debug logger. */
    void log(DebugSeverity severity, const char* msg);

    /* Initialize the VRAM bank for this GPU adapter. */
    Result<void> init_vram_bank();

    /* ===== Platform-specific ===== */
public:
    /* Initialize the GPU adapter. */
    Result<void> init(bool debug_mode = false);
    
    /* Get the VRAM bank for this GPU adapter. */
    VRAMBank& get_vram_bank();

    /* Destroy the GPU adapter, free all its resources. */
    Result<void> destroy();
};
