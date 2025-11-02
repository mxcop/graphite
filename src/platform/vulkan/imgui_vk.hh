#pragma once

/* Interface header */
#include "graphite/imgui.hh"

#include "vulkan/api_vk.hh" /* Vulkan API */

/**
 * Immediate Mode GUI.  
 * Used for rendering debug/editor UI.
 */
class ImGUI : public AgnImGUI {
    /* ImGUI resources */
    VkDescriptorPool desc_pool {};

public:
    /* Initialize the immediate mode GUI. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu, RenderTarget rt);
    
    /* Destroy the immediate mode GUI, free all its resources. */
    PLATFORM_SPECIFIC Result<void> destroy();
};
