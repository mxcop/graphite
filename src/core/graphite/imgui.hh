#pragma once

#include "platform/platform.hh"

#include "resources/handle.hh"
#include "utils/result.hh"
#include "utils/debug.hh"

class GPUAdapter;

struct ImGUIFunctions {
    /* e.g. ImGui_ImplVulkan_Init */
    void* graphics_init {};
    /* e.g. ImGui_ImplVulkan_NewFrame */
    void* new_frame {};
    /* e.g. ImGui_ImplVulkan_Shutdown */
    void* graphics_shutdown {};
};

/**
 * @warning Never use this class directly!
 * This is an interface for the platform-specific class.
 */
class AgnImGUI {
protected:
    GPUAdapter* gpu = nullptr;

    /* Collection of imgui functions. */
    ImGUIFunctions functions {};

public:
    /* Initialize the immediate mode GUI. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu, RenderTarget rt, ImGUIFunctions functions) = 0;
    
    /* Start a new immediate frame. */
    PLATFORM_SPECIFIC void new_frame() = 0;
    
    /* Destroy the immediate mode GUI, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy() = 0;
};

#include PLATFORM_INCLUDE(imgui)
