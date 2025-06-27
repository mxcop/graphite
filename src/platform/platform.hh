#pragma once

/* Some default platform defines */
#ifndef PLATFORM
#define PLATFORM vulkan
#endif
#ifndef PLATFORM_EXT
#define PLATFORM_EXT vk
#endif

/* Concatenation macros */
#define __CAT(X,Y) X##Y
#define CAT(X,Y) __CAT(X,Y)
#define CAT_7(A, B, C, D, E, F, G) CAT(A, CAT(B, CAT(C, CAT(D, CAT(E, CAT(F, G))))))

/* Stringification macros */
#define __STR(X) #X
#define STR(X) __STR(X)

/* Platform-specific file include macros */
#define PLATFORM_H(BASE) STR(CAT_7(platform/, PLATFORM, /, BASE, _, PLATFORM_EXT, .hh))
// #define PLATFORM_C(BASE) STR(CAT_7(platform/, PLATFORM, /, BASE, _, PLATFORM_EXT, .cc))
