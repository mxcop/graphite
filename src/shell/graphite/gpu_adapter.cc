#include "gpu_adapter.hh"

int GPUAdapter::agn_function() {
    return test;
}

/* Include platform-specific implementation */
#include PLATFORM_C(gpu_adapter)
