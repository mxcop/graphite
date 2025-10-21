#include "debug.hh"

#include <cstdarg>

/* Returns true if the severity should be logged given the level. */
bool passes_level(const DebugLevel level, const DebugSeverity severity) {
    /* Let all messages through */
    if (level == DebugLevel::Verbose) return true;

    /* Block all info messages */
    if (severity == DebugSeverity::Info) return false;
    if (level == DebugLevel::Warning) return true;

    /* Block all warning messages */
    if (severity == DebugSeverity::Warning) return false;
    return true; /* Always let errors through */
}

/* An empty debug logger, won't do anything. */
void empty_logger(const DebugSeverity, const char *, void *) {}

/* Default debug logger, simply uses printf to print out a message. */
void default_logger(const DebugSeverity severity, const char *msg, void *) {
    switch (severity) {
    case DebugSeverity::Info:
        printf("[graphics] info: %s\n", msg);
        break;
    case DebugSeverity::Warning:
        printf("[graphics] warning: %s\n", msg);
        break;
    case DebugSeverity::Error:
        printf("[graphics] error: %s\n", msg);
        break;
    }
}

/* Default colored debug logger, simply uses printf to print out a message. */
void color_logger(const DebugSeverity severity, const char *msg, void *) {
    switch (severity) {
    case DebugSeverity::Info:
        printf("\033[2m[graphite]\033[0m \033[1;3;36minfo\033[0m: %s\n", msg);
        break;
    case DebugSeverity::Warning:
        printf("\033[2m[graphite]\033[0m \033[1;3;33mwarning\033[0m: %s\n", msg);
        break;
    case DebugSeverity::Error:
        printf("\033[2;31m[graphite]\033[0m \033[1;31merror\033[0m: \033[31m%s\033[0m\n", msg);
        break;
    }
}

/* Format a string with arguments. (like: printf) */
std::string strfmt(const std::string_view fmt, ...) {
    int size = ((int)fmt.size()) * 2 + 50;
    std::string str {};
    va_list ap;
    for (;;) {
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char*)str.data(), size, fmt.data(), ap);
        va_end(ap);
        if (n > -1 && n < size) {
            str.resize(n);
            break;
        }
        if (n > -1)
            size = n + 1;
        else
            size *= 2;
    }
    return str;
}
