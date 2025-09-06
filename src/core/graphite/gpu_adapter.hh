#pragma once

#include "platform/platform.hh"

#include "utils/result.hh"
#include "utils/debug.hh"

class VRAMBank;

/* Debug logger data. */
struct DebugLogger {
    DebugLoggerFn logger = nullptr;
    DebugLevel log_level = DebugLevel::Warning;
    void* user_data = nullptr;
};

/**
 * @warning Never use this class directly!
 * This is an interface for the platform-specific class.
 */
class AgnGPUAdapter {
protected:
    /* Active debug logger. */
    DebugLogger logger {empty_logger, DebugLevel::Warning, nullptr};

    /* VRAM bank for this GPU adapter. */
    VRAMBank* vram_bank = nullptr;
    
    /* Log a message using the active debug logger. */
    void log(DebugSeverity severity, const char* msg);

    /* Initialize the VRAM bank for this GPU adapter. */
    Result<void> init_vram_bank();

public:
    /* Initialize the GPU adapter. */
    PLATFORM_SPECIFIC Result<void> init(bool debug_mode = false) = 0;
    
    /* Set the debug logger callback. */
    void set_logger(DebugLoggerFn callback = default_logger, DebugLevel level = DebugLevel::Warning, void* user_data = nullptr);

    /* Get the VRAM bank for this GPU adapter. */
    VRAMBank& get_vram_bank();

    /* Destroy the GPU adapter, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy() = 0;
};

#include PLATFORM_INCLUDE(gpu_adapter)
