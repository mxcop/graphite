#include "vram_bank_vk.hh"

Result<void> VRAMBank::destroy() {
    return Ok();
}

Result<RenderTarget> VRAMBank::create_render_target(TargetDesc& target, u32 def_width, u32 def_height) {
    /* Pop a new render target off the stock */
    StockPair resource = render_targets.pop();

    // VkFormat surface_fmt {};
    // VkColorSpaceKHR surface_cs {};
    // VkSwapchainKHR swapchain {};

    /* Create a KHR surface */
    VkWin32SurfaceCreateInfoKHR surface_ci { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    surface_ci.hwnd = target.window;
    if (vkCreateWin32SurfaceKHR(gpu->instance, &surface_ci, nullptr, &resource.data.surface) != VK_SUCCESS) {
        return Err("failed to create win32 khr surface.");
    }

    return Ok(resource.handle);
}

void VRAMBank::destroy_render_target(RenderTarget &render_target) {
    /* Push the handle back onto the stock, and get its slot for cleanup */
    RenderTargetSlot& slot = render_targets.push(render_target);

    /* Destroy the KHR surface */
    vkDestroySurfaceKHR(gpu->instance, slot.surface, nullptr);
}
