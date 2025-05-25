#pragma once

#include "platform/platform.hh"

#include "result.hh"
#include "debug.hh"

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

public: /* Platform-agnostic functions */
    /* Set the debug logger callback. */
    void set_logger(DebugLoggerFn callback = default_logger, DebugLevel level = DebugLevel::Warning, void* user_data = nullptr);

private:
    /* Log a message using the active debug logger. */
    void log(DebugSeverity severity, const char* msg);

public: /* Platform-specific functions */
    /* Initialize the GPU adapter. */
    Result<void> init(bool debug_mode = false);

    /* Destroy the GPU adapter, free all its resources. */
    Result<void> destroy();
};
