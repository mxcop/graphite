#pragma once

#ifdef GRAPHITE_IMGUI

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
    VkSampler bilinear_sampler {};
    
    /* Render a frame using immediate mode GUI. */
    void render(VkCommandBuffer cmd);

public:
    /* Initialize the immediate mode GUI. */
    PLATFORM_SPECIFIC Result<void> init(GPUAdapter& gpu, RenderTarget rt);
    
    /* Start a new immediate frame. */
    PLATFORM_SPECIFIC void new_frame();

    /* Add an image resource to the immediate mode GUI. */
    PLATFORM_SPECIFIC u64 add_image(Image image);
    
    /* Remove an image resource from the immediate mode GUI. */
    PLATFORM_SPECIFIC void remove_image(Image image);

    /* De-initialize the immediate mode GUI, free all its resources. */
    PLATFORM_SPECIFIC Result<void> deinit();

    /* To access 'render()' function. */
    friend class RenderGraph;
};

#endif
