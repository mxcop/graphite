#include "gpu_adapter.hh"

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
