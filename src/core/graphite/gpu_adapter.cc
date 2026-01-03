#include "gpu_adapter.hh"

#include "vram_bank.hh"

/* Set the debug logger callback. */
void AgnGPUAdapter::set_logger(DebugLoggerFn callback, DebugLevel level, void *user_data) {
    logger.logger = callback;
    logger.log_level = level;
    logger.user_data = user_data;
}

/* Log a message using the active debug logger. */
void AgnGPUAdapter::log(DebugSeverity severity, const char *msg) {
    /* Make sure there is a logger */
    if (logger.logger == nullptr) return;

    /* Check if the severity passes the log level */
    if (passes_level(logger.log_level, severity) == false) return;

    logger.logger(severity, msg, logger.user_data);
}

/* Initialize the VRAM bank for this GPU adapter. */
Result<void> AgnGPUAdapter::init_vram_bank() {
    /* Safety check */
    if (vram_bank != nullptr) return Err("tried to initialize vram bank which has already been initialized.");

    /* Allocate & initialize VRAM bank */
    vram_bank = new VRAMBank {};
    if (const Result r = vram_bank->init(*(GPUAdapter*)this); r.is_err()) {
        return Err(r.unwrap_err());
    }
    return Ok();
}

/* De-initialize the VRAM bank for this GPU adapter. */
void AgnGPUAdapter::deinit_vram_bank() { delete vram_bank; }

/* Get the VRAM bank for this GPU adapter. */
VRAMBank& AgnGPUAdapter::get_vram_bank() {
    return *vram_bank;
}

void AgnGPUAdapter::set_max_render_targets(const u32 count) { max_render_targets = count; }
void AgnGPUAdapter::set_max_buffers(const u32 count) { max_buffers = count; }
void AgnGPUAdapter::set_max_textures(const u32 count) { max_textures = count; }
void AgnGPUAdapter::set_max_images(const u32 count) { max_images = count; }
void AgnGPUAdapter::set_max_samplers(const u32 count) { max_samplers = count; }

u32 AgnGPUAdapter::get_max_render_targets() const { return max_render_targets; }
u32 AgnGPUAdapter::get_max_buffers() const { return max_buffers; }
u32 AgnGPUAdapter::get_max_textures() const { return max_textures; }
u32 AgnGPUAdapter::get_max_images() const { return max_images; }
u32 AgnGPUAdapter::get_max_samplers() const { return max_samplers; }
