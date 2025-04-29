#pragma once

#include "platform/platform.hh"

struct ImplTestAdapter;

#ifndef PLATFORM_IMPL
/* Include platform-specific "ImplTestAdapter" struct */
#include PLATFORM_H(test)
#endif

class TestAdapter : ImplTestAdapter {
    int test = 21;

public: /* Platform-agnostic functions */
    int agn_function();

public: /* Platform-specific functions */
    void spc_function();
};
