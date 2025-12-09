#pragma once

#include <unordered_map>

#include "platform/platform.hh"

#include "resources/handle.hh"
#include "utils/result.hh"
#include "utils/debug.hh"
#include "utils/types.hh"

class GPUAdapter;

struct ImGUIFunctions {
    /* e.g. ImGui_ImplVulkan_Init */
    void* graphics_init {};
    /* e.g. ImGui_ImplVulkan_AddTexture */
    void* add_texture {};
    /* e.g. ImGui_ImplVulkan_RemoveTexture */
    void* remove_texture {};
    /* e.g. ImGui_ImplVulkan_NewFrame */
    void* new_frame {};
    /* ImGui::GetDrawData */
    void* draw_data {};
    /* e.g. ImGui_ImplVulkan_RenderDrawData */
    void* render {};
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
    bool clear_screen = false;

    /* Collection of imgui functions. */
    ImGUIFunctions functions {};

    /* Map of image resources mapped to imgui. */
    std::unordered_map<u32, u64> image_map {};

public:
    /* Initialize the immediate mode GUI. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu, RenderTarget rt, ImGUIFunctions functions) = 0;

    /* Set whether the immediate mode GUI should clear the screen before rendering. (default: false) */
    void set_clear_screen(bool value = false) { clear_screen = value; };

    /* Start a new immediate frame. */
    PLATFORM_SPECIFIC void new_frame() = 0;
    
    /* Add an image resource to the immediate mode GUI. */
    PLATFORM_SPECIFIC u64 add_image(Image image) = 0;
    
    /* Get the image index of an image resource. */
    u64 get_image(Image image) { return image_map[image.get_index()]; };
    
    /* Remove an image resource from the immediate mode GUI. */
    PLATFORM_SPECIFIC void remove_image(Image image) = 0;
    
    /* De-initialize the immediate mode GUI, free all its resources. */
    PLATFORM_SPECIFIC Result<void> deinit() = 0;
};

#include PLATFORM_INCLUDE(imgui)
