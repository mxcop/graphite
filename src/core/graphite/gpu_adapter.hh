#pragma once

#include "platform/platform.hh"

#include "utils/result.hh"
#include "utils/debug.hh"

#include "utils/types.hh"

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

    /* Maximum amount of resources. */
    u32 max_render_targets = 8u;
    u32 max_buffers = 8u;
    u32 max_textures = 8u;
    u32 max_images = 8u;
    u32 max_samplers = 8u;
    
    /* Log a message using the active debug logger. */
    void log(DebugSeverity severity, const char* msg);

    /* Initialize the VRAM bank for this GPU adapter. */
    Result<void> init_vram_bank();

    /* De-initialize the VRAM bank for this GPU adapter. */
    void deinit_vram_bank();

public:
    /* Initialize the GPU adapter. */
    PLATFORM_SPECIFIC Result<void> init(bool debug_mode = false) = 0;
    
    /* Set the debug logger callback. */
    void set_logger(DebugLoggerFn callback = default_logger, DebugLevel level = DebugLevel::Warning, void* user_data = nullptr);

    /* Get the VRAM bank for this GPU adapter. */
    VRAMBank& get_vram_bank();

    /* De-initialize the GPU adapter, free all its resources. */
    PLATFORM_SPECIFIC Result<void> deinit() = 0;

    /* Set maximum resource count. */
    void set_max_render_targets(const u32 count);
    void set_max_buffers(const u32 count);
    void set_max_textures(const u32 count);
    void set_max_images(const u32 count);
    void set_max_samplers(const u32 count);

    /* Get maximum resource count. */
    u32 get_max_render_targets() const;
    u32 get_max_buffers() const;
    u32 get_max_textures() const;
    u32 get_max_images() const;
    u32 get_max_samplers() const;
};

#include PLATFORM_INCLUDE(gpu_adapter)
