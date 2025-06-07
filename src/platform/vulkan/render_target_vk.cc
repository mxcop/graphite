#include "render_target_vk.hh"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h> /* Windows API */
#endif

/* Initialize the Render Target. */
Result<void> RenderTarget::init(GPUAdapter& gpu, TargetDesc& target, uint32_t def_width, uint32_t def_height) {
    width = def_width;
    height = def_height;
    
#if defined(_WIN32) || defined(_WIN64)
    /* Check if the window handle is valid */
    if (target.window == NULL || IsWindow(target.window) == false) {
        return Err("given hwnd for win32 surface was not a valid window.");
    }
    VkWin32SurfaceCreateInfoKHR surface_ci { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    /* <https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowlongptra> */
    surface_ci.hinstance = (HINSTANCE)GetWindowLongPtr(target.window, GWLP_HINSTANCE);
    surface_ci.hwnd = target.window;
    if (vkCreateWin32SurfaceKHR(gpu.instance, &surface_ci, nullptr, &surface) != VK_SUCCESS) {
        return Err("failed to create win32 surface for render target.");
    }
#else
    #error "TODO: provide surface creation code for this platform!"
#endif

    return rebuild(); /* Trigger a re-build */
}

/* Re-build the Render Target. */
Result<void> RenderTarget::rebuild() {

    /* TODO: Destroy previous swapchain, init the new swapchain, create image views. */

    return Ok();
}

/* Destroy the Render Target, free all its resources. */
Result<void> RenderTarget::destroy(GPUAdapter& gpu) {
    vkDestroySurfaceKHR(gpu.instance, surface, nullptr);
    return Ok();
}
