#pragma once

#include <cstdio>
#include <string>

/* Debug message severtiy. */
enum class DebugSeverity {
    Info, Warning, Error
};

/* Debug message level. */
enum class DebugLevel {
    Verbose, /* Log all messages. */
    Warning, /* Log only warnings & error. */
    Error    /* Log only errors. */
};

/* Debug logger function signature. */
typedef void (*DebugLoggerFn)(const DebugSeverity severity, const char* msg, void* user_data);

/* Returns true if the severity should be logged given the level. */
bool passes_level(const DebugLevel level, const DebugSeverity severity);

/* An empty debug logger, won't do anything. */
void empty_logger(const DebugSeverity, const char*, void*);

/* Default debug logger, simply uses printf to print out a message. */
void default_logger(const DebugSeverity severity, const char* msg, void*);

/* Default colored debug logger, simply uses printf to print out a message. */
void color_logger(const DebugSeverity severity, const char* msg, void*);

/* Format a string with arguments. (like: printf) */
std::string strfmt(const std::string_view fmt, ...);
