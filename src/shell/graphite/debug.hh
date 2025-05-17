// #pragma once

// #include <cstdio>

// /* Debug message severtiy. */
// enum class DebugSeverity {
//     Warning, Error
// };

// /* Debug callback function signature. */
// typedef void (*DebugCallback)(const DebugSeverity severity, const char* msg, void* user_data);

// /* An empty debug callback, won't do anything, should get optimized away. */
// void empty_debug_callback(const DebugSeverity, const char*, void*) {};

// /* Default debug callback, simply uses printf to print out a message. */
// void default_debug_callback(const DebugSeverity severity, const char* msg, void*) {
//     switch (severity) {
//     case DebugSeverity::Warning:
//         printf("[graphite] warn: %s\n", msg);
//         break;
//     case DebugSeverity::Error:
//         printf("[graphite] err: %s\n", msg);
//         break;
//     }
// }
