#pragma once

// #include <imgui.h>

#include "platform/platform.hh"

#include "resources/handle.hh"
#include "utils/result.hh"
#include "utils/debug.hh"

class GPUAdapter;

struct ImGUIFunctions {
    /* e.g. ImGui_ImplVulkan_Init */
    void* GraphicsInit {};
};

/**
 * @warning Never use this class directly!
 * This is an interface for the platform-specific class.
 */
class AgnImGUI {
protected:
    GPUAdapter* gpu = nullptr;

    ImGUIFunctions functions {};

public:
    /* Initialize the immediate mode GUI. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu, RenderTarget rt, ImGUIFunctions functions) = 0;
    
    /* Destroy the immediate mode GUI, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy() = 0;
};

#include PLATFORM_INCLUDE(imgui)
