#pragma once

#ifdef GRAPHITE_IMGUI

#include <unordered_map>

#include <imgui.h> /* ImGUI header */

#include "platform/platform.hh"

#include "resources/handle.hh"
#include "utils/result.hh"
#include "utils/debug.hh"
#include "utils/types.hh"

class GPUAdapter;

/**
 * @warning Never use this class directly!
 * This is an interface for the platform-specific class.
 */
class AgnImGUI {
protected:
    GPUAdapter* gpu = nullptr;
    bool clear_screen = false;

    /* Map of image resources mapped to imgui. */
    std::unordered_map<u32, u64> image_map {};

public:
    /* Initialize the immediate mode GUI. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu, RenderTarget rt) = 0;

    /* Set whether the immediate mode GUI should clear the screen before rendering. (default: false) */
    void set_clear_screen(bool value = false) { clear_screen = value; };

    /* Start a new immediate frame. */
    PLATFORM_SPECIFIC void new_frame() = 0;
    
    /* Add an image resource to the immediate mode GUI. */
    PLATFORM_SPECIFIC u64 add_image(Image image) = 0;
    
    /* Get the image index of an image resource. */
    u64 get_image(Image image) { return image_map[image.raw()]; };
    
    /* Remove an image resource from the immediate mode GUI. */
    PLATFORM_SPECIFIC void remove_image(Image image) = 0;
    
    /* De-initialize the immediate mode GUI, free all its resources. */
    PLATFORM_SPECIFIC Result<void> deinit() = 0;
};

#include PLATFORM_INCLUDE(imgui)

#endif
