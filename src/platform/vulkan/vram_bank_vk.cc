#include "vram_bank_vk.hh"

#define MAX(A, B) ((A > B) ? A : B)
#define MIN(A, B) ((A < B) ? A : B)

Result<void> VRAMBank::destroy() {
    return Ok();
}

Result<RenderTarget> VRAMBank::create_render_target(const TargetDesc& target, u32 width, u32 height) {
    /* Pop a new render target off the stock */
    StockPair resource = render_targets.pop();

    /* Create a KHR surface */
    VkWin32SurfaceCreateInfoKHR surface_ci { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    surface_ci.hwnd = target.window;
    if (vkCreateWin32SurfaceKHR(gpu->instance, &surface_ci, nullptr, &resource.data.surface) != VK_SUCCESS) {
        return Err("failed to create win32 surface.");
    }

    /* Get the capabilities of the KHR surface */
    VkSurfaceCapabilitiesKHR surface_features {};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->physical_device, resource.data.surface, &surface_features) != VK_SUCCESS) {
        return Err("failed to get surface capabilities.");
    }

    /* Use up to 4 images that are available, this allows for triple buffering */
    resource.data.image_count = MIN(4, MAX(1, surface_features.maxImageCount));

    /* Set the width and height, with respect to the surface limits */
    resource.data.extent.width = MIN(MAX(width, surface_features.minImageExtent.width), surface_features.maxImageExtent.width);
    resource.data.extent.height = MIN(MAX(width, surface_features.minImageExtent.height), surface_features.maxImageExtent.height);

    /* Get the available surface formats */
    u32 format_count = 0u;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu->physical_device, resource.data.surface, &format_count, nullptr);
    VkSurfaceFormatKHR* formats = new VkSurfaceFormatKHR[format_count] {};
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu->physical_device, resource.data.surface, &format_count, formats);

    /* Find an RGBA8 unorm format */
    for (u32 i = 0u; i < format_count; ++i) {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) {
            resource.data.format = formats[i].format;
            resource.data.color_space = formats[i].colorSpace;
            break;
        }
    }
    delete[] formats; /* Free the formats */

    /* Make sure we found the format we need */
    if (resource.data.format != VK_FORMAT_R8G8B8A8_UNORM) {
        return Err("failed to find rgba unorm surface format.");
    }

    /* Swapchain creation info */
    VkSwapchainCreateInfoKHR swapchain_ci { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_ci.surface = resource.data.surface;
    swapchain_ci.minImageCount = resource.data.image_count;
    swapchain_ci.imageFormat = resource.data.format;
    swapchain_ci.imageColorSpace = resource.data.color_space;
    swapchain_ci.imageExtent = resource.data.extent;
    swapchain_ci.imageArrayLayers = 1u;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_ci.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; /* TODO: Vsync parameter. */
    swapchain_ci.clipped = true;

    /* Create the swapchain */
    if (vkCreateSwapchainKHR(gpu->logical_device, &swapchain_ci, nullptr, &resource.data.swapchain) != VK_SUCCESS) {
        return Err("failed to create swapchain for render target.");
    }

    /* Get images from the swapchain */
    resource.data.images = new VkImage[resource.data.image_count] {};
    if (vkGetSwapchainImagesKHR(gpu->logical_device, resource.data.swapchain, &resource.data.image_count, resource.data.images) != VK_SUCCESS) {
        return Err("failed to get swapchain images for render target.");
    }

    /* Semaphore creation info */
    const VkSemaphoreCreateInfo sema_ci { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    /* Create an image view for each swapchain image */
    resource.data.semaphores = new VkSemaphore[resource.data.image_count] {};
    resource.data.views = new VkImageView[resource.data.image_count] {};
    resource.data.old_layouts = new VkImageLayout[resource.data.image_count] {};
    for (u32 i = 0u; i < resource.data.image_count; ++i) {
        /* Image view creation info */
        VkImageViewCreateInfo view_ci { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_ci.image = resource.data.images[i];
        view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_ci.format = resource.data.format;
        view_ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u };
        resource.data.old_layouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;

        /* Create image view */
        if (vkCreateImageView(gpu->logical_device, &view_ci, nullptr, &resource.data.views[i]) != VK_SUCCESS) {
            return Err("failed to create image view for render target.");
        }
        /* Create image presentation semaphore */
        if (vkCreateSemaphore(gpu->logical_device, &sema_ci, nullptr, &resource.data.semaphores[i]) != VK_SUCCESS) {
            return Err("failed to create semaphore for render target.");
        }
    }

    return Ok(resource.handle);
}

Result<void> VRAMBank::resize_render_target(RenderTarget &render_target, u32 width, u32 height) {
    /* Wait for the queue to idle */
    vkQueueWaitIdle(gpu->queues.queue_combined);
    RenderTargetSlot& data = render_targets.get(render_target);

    /* Get the capabilities of the KHR surface */
    VkSurfaceCapabilitiesKHR surface_features {};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->physical_device, data.surface, &surface_features) != VK_SUCCESS) {
        return Err("failed to get surface capabilities.");
    }

    /* Set the width and height, with respect to the surface limits */
    data.extent.width = MIN(MAX(width, surface_features.minImageExtent.width), surface_features.maxImageExtent.width);
    data.extent.height = MIN(MAX(width, surface_features.minImageExtent.height), surface_features.maxImageExtent.height);

    /* Swapchain re-creation info */
    VkSwapchainCreateInfoKHR swapchain_ci { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_ci.surface = data.surface;
    swapchain_ci.minImageCount = data.image_count;
    swapchain_ci.imageFormat = data.format;
    swapchain_ci.imageColorSpace = data.color_space;
    swapchain_ci.imageExtent = data.extent;
    swapchain_ci.imageArrayLayers = 1u;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_ci.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; /* TODO: Vsync parameter. */
    swapchain_ci.clipped = true;
    swapchain_ci.oldSwapchain = data.swapchain;

    /* Create the swapchain */
    if (vkCreateSwapchainKHR(gpu->logical_device, &swapchain_ci, nullptr, &data.swapchain) != VK_SUCCESS) {
        return Err("failed to re-create swapchain for render target.");
    }

    /* Destroy the old swapchain */
    vkDestroySwapchainKHR(gpu->logical_device, swapchain_ci.oldSwapchain, nullptr);

    /* Get images from the swapchain */
    if (vkGetSwapchainImagesKHR(gpu->logical_device, data.swapchain, &data.image_count, data.images) != VK_SUCCESS) {
        return Err("failed to get swapchain images for render target.");
    }

    /* Create an image view for each swapchain image */
    for (u32 i = 0u; i < data.image_count; ++i) {
        /* Image view creation info */
        VkImageViewCreateInfo view_ci { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_ci.image = data.images[i];
        view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_ci.format = data.format;
        view_ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u };
        data.old_layouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;

        /* Destroy old image view */
        vkDestroyImageView(gpu->logical_device, data.views[i], nullptr);

        /* Create new image view */
        if (vkCreateImageView(gpu->logical_device, &view_ci, nullptr, &data.views[i]) != VK_SUCCESS) {
            return Err("failed to create image view for render target.");
        }
    }

    return Ok();
}

void VRAMBank::destroy_render_target(RenderTarget &render_target) {
    /* Wait for the queue to idle */
    vkQueueWaitIdle(gpu->queues.queue_combined);
    
    /* Push the handle back onto the stock, and get its slot for cleanup */
    RenderTargetSlot& slot = render_targets.push(render_target);

    /* Destroy the images & views */
    delete[] slot.images;
    delete[] slot.old_layouts;
    for (u32 i = 0u; i < slot.image_count; ++i) {
        vkDestroyImageView(gpu->logical_device, slot.views[i], nullptr);
        vkDestroySemaphore(gpu->logical_device, slot.semaphores[i], nullptr);
    }
    delete[] slot.views;
    delete[] slot.semaphores;

    /* Destroy the swapchain */
    vkDestroySwapchainKHR(gpu->logical_device, slot.swapchain, nullptr);

    /* Destroy the KHR surface */
    vkDestroySurfaceKHR(gpu->instance, slot.surface, nullptr);
}
