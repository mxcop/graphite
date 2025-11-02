#pragma once

#include <imgui.h>

#include "platform/platform.hh"

#include "resources/handle.hh"
#include "utils/result.hh"
#include "utils/debug.hh"

class GPUAdapter;

/**
 * @warning Never use this class directly!
 * This is an interface for the platform-specific class.
 */
class AgnImGUI {
protected:
    GPUAdapter* gpu = nullptr;

public:
    /* Initialize the immediate mode GUI. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu, RenderTarget rt) = 0;
    
    /* Destroy the immediate mode GUI, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy() = 0;
};

#include PLATFORM_INCLUDE(imgui)
