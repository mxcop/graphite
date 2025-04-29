#include "gpu_adapter_vk.hh"

#define PLATFORM_IMPL
#include "graphite/gpu_adapter.hh"

#include <cstdio>

void GPUAdapter::spc_function() {
    printf("number b: %i\n", number);
}
