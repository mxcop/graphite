#include "render_target_vk.hh"

/* Initialize the Render Target. */
Result<void> RenderTarget::init(void* target, uint32_t def_width, uint32_t def_height) {
    width = def_width;
    height = def_height;
    surface = (VkSurfaceKHR)target;
    return rebuild(); /* Trigger a re-build */
}

/* Re-build the Render Target. */
Result<void> RenderTarget::rebuild() {

    /* TODO: Destroy previous swapchain, init the new swapchain, create image views. */

    return Ok();
}
