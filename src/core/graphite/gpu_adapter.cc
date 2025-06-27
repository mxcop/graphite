#include "gpu_adapter.hh"

#include "vram_bank.hh"

/* Set the debug logger callback. */
void GPUAdapter::set_logger(DebugLoggerFn callback, DebugLevel level, void *user_data) {
    logger.logger = callback;
    logger.log_level = level;
    logger.user_data = user_data;
}

/* Log a message using the active debug logger. */
void GPUAdapter::log(DebugSeverity severity, const char *msg) {
    /* Make sure there is a logger */
    if (logger.logger == nullptr) return;

    /* Check if the severity passes the log level */
    if (passes_level(logger.log_level, severity) == false) return;

    logger.logger(severity, msg, logger.user_data);
}

Result<void> GPUAdapter::init_vram_banks(u32 count) {
    /* Safety checks */
    if (count == 0u) return Err("vram bank count cannot be zero. (gpu adapter)");
    if (count > 16u) return Err("vram bank count cannot be more than 16. (gpu adapter)");
    vram_bank_count = count;

    /* Free existing VRAM banks if any */
    if (vram_banks != nullptr) {
        delete[] vram_banks;
        vram_banks = nullptr;
    }

    /* Allocate & initialize VRAM banks */
    vram_banks = new VRAMBank[count] {};
    for (u32 i = 0u; i < count; ++i) {
        if (const Result r = vram_banks[i].init(*this, i); r.is_err()) {
            return Err(r.unwrap_err().c_str());
        }
    }
    return Ok();
}

Result<VRAMBank*> GPUAdapter::get_vram_bank(u32 bank_index) {
    if (bank_index >= vram_bank_count) return Err("tried to get vram bank that was out of range.");
    return Ok(&vram_banks[bank_index]);
}
