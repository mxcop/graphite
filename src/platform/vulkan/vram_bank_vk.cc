#define VMA_IMPLEMENTATION
#include "vram_bank_vk.hh"

#include "wrapper/translate_vk.hh"

#define MAX(A, B) ((A > B) ? A : B)
#define MIN(A, B) ((A < B) ? A : B)

Result<void> VRAMBank::init(GPUAdapter& gpu) {
    this->gpu = &gpu;

    { /* Initialize VMA */
        VmaVulkanFunctions vulkan_functions{};

        VmaAllocatorCreateInfo vma_info{};
        vma_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        vma_info.vulkanApiVersion = VK_API_VERSION;
        vma_info.physicalDevice = gpu.physical_device;
        vma_info.device = gpu.logical_device;
        vma_info.instance = gpu.instance;

        if (vmaImportVulkanFunctionsFromVolk(&vma_info, &vulkan_functions) != VK_SUCCESS)
            return Err("Failed to import vulakn functions from volk (required for VMA init).");

        vma_info.pVulkanFunctions = &vulkan_functions;

        if (vmaCreateAllocator(&vma_info, &vma_allocator) != VK_SUCCESS)
            return Err("Failed to initialise VMA.");
    }

    /* Initialize the Stack Pools */
    render_targets.init(8u);
    buffers.init(8u);
    textures.init(8u);
    images.init(8u);
    samplers.init(8u);

    /* Command pool creation info */
    VkCommandPoolCreateInfo pool_ci { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_ci.queueFamilyIndex = gpu.queue_families.queue_transfer;

    /* Create command pool for command buffers */
    if (vkCreateCommandPool(gpu.logical_device, &pool_ci, nullptr, &upload_cmd_pool) != VK_SUCCESS) {
        return Err("failed to create command pool for render graph.");
    }

    /* Allocate upload command buffer */
    VkCommandBufferAllocateInfo cmd_ai { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmd_ai.commandPool = upload_cmd_pool;
    cmd_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_ai.commandBufferCount = 1u;

    if (vkAllocateCommandBuffers(gpu.logical_device, &cmd_ai, &upload_cmd) != VK_SUCCESS) {
        return Err("failed to create upload command buffer.");
    }

    /* Create the upload fence */
    VkFenceCreateInfo fence_ci { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    if (vkCreateFence(gpu.logical_device, &fence_ci, nullptr, &upload_fence) != VK_SUCCESS) {
        return Err("failed to create upload fence");
    }

    return Ok();
}

bool VRAMBank::begin_upload() {
    VkCommandBufferBeginInfo begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    return vkBeginCommandBuffer(upload_cmd, &begin_info) == VK_SUCCESS;
}

bool VRAMBank::end_upload() {
    /* End the upload command buffer */
    vkEndCommandBuffer(upload_cmd);

    /* Submit the upload commands to the queue */
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo submit { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1u;
    submit.pCommandBuffers = &upload_cmd;

    if (vkQueueSubmit(gpu->queues.queue_transfer, 1u, &submit, upload_fence) != VK_SUCCESS) {
        return false;
    }

    /* Wait for the upload commands to complete */
    if (vkWaitForFences(gpu->logical_device, 1u, &upload_fence, true, UINT64_MAX) != VK_SUCCESS) return false;
    if (vkResetFences(gpu->logical_device, 1u, &upload_fence) != VK_SUCCESS) return false;
    return true;
}

Result<void> VRAMBank::destroy() {
    vkDestroyCommandPool(gpu->logical_device, upload_cmd_pool, nullptr);

    vkDestroyFence(gpu->logical_device, upload_fence, nullptr);

    vmaDestroyAllocator(vma_allocator);
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
    swapchain_ci.presentMode = VK_PRESENT_MODE_MAILBOX_KHR; /* TODO: Vsync parameter. */
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

Result<Buffer> VRAMBank::create_buffer(BufferUsage usage, u64 count, u64 stride) {
    /* Make sure the buffer usage is valid */
    if (usage == BufferUsage::Invalid) return Err("invalid buffer usage.");

    /* Pop a new buffer off the stock */
    StockPair resource = buffers.pop();
    resource.data.usage = usage;

    /* Size of the buffer in bytes */
    const u64 size = stride == 0 ? count : count * stride;

    /* Buffer creation info */
    VkBufferCreateInfo buffer_ci { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_ci.size = size;
    buffer_ci.usage = translate::buffer_usage(usage);
    buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_ci.queueFamilyIndexCount = 1u;
    buffer_ci.pQueueFamilyIndices = &gpu->queue_families.queue_combined;

    /* Memory allocation info */
    VmaAllocationCreateInfo alloc_ci {};
    alloc_ci.flags = 0x00u;
    alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;

    /* Create the buffer & allocate it using VMA */
    if (vmaCreateBuffer(vma_allocator, &buffer_ci, &alloc_ci, &resource.data.buffer, &resource.data.alloc, nullptr) != VK_SUCCESS) { 
        return Err("failed to create buffer.");
    }
    return Ok(resource.handle);
}

Result<Texture> VRAMBank::create_texture(TextureUsage usage, TextureFormat fmt, Size3D size, TextureMeta meta) {
    /* Make sure the texture usage is valid */
    if (usage == TextureUsage::Invalid) return Err("invalid texture usage.");

    /* Pop a new texture off the stock */
    StockPair resource = textures.pop();
    resource.data.usage = usage;
    resource.data.format = fmt;
    resource.data.size = size;
    resource.data.meta = meta;

    const VkFormat format = translate::texture_format(fmt);

    /* Image creation info */
    VkImageCreateInfo texture_ci { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    texture_ci.imageType = size.is_2d() ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
    texture_ci.format = format;
    texture_ci.extent = { MAX(size.x, 1u), MAX(size.y, 1u), MAX(size.z, 1u) };
    texture_ci.mipLevels = MAX(1u, meta.mips);
    texture_ci.arrayLayers = MAX(1u, meta.arrays);
    texture_ci.samples = VK_SAMPLE_COUNT_1_BIT; /* No MSAA */
    texture_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    texture_ci.usage = translate::texture_usage(usage);
    texture_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    /* Memory allocation info */
    VmaAllocationCreateInfo alloc_ci {};
    alloc_ci.flags = 0x00u; 
    alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;

    /* Create the texture & allocate it using VMA */
    if (vmaCreateImage(vma_allocator, &texture_ci, &alloc_ci, &resource.data.image, &resource.data.alloc, nullptr) != VK_SUCCESS) { 
        return Err("failed to allocate image resource.");
    }
    return Ok(resource.handle);
}

Result<Image> VRAMBank::create_image(Texture texture, u32 mip, u32 layer) {
    /* Make sure the texture is valid */
    if (texture.is_null()) return Err("cannot create image for texture which is null.");

    /* Pop a new image off the stock */
    StockPair resource = images.pop();
    resource.data.texture = texture;
    TextureSlot& texture_slot = textures.get(texture);
    
    /* Image access sub resource range */
    VkImageSubresourceRange sub_range {};
    sub_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; /* Color hardcoded! (might want depth too) */
    sub_range.baseMipLevel = mip;
    sub_range.levelCount = MAX(1u, texture_slot.meta.mips - mip);
    sub_range.baseArrayLayer = layer;
    sub_range.layerCount = MAX(1u, texture_slot.meta.arrays - layer);
    resource.data.sub_range = sub_range;

    /* Image view creation info */
    VkImageViewCreateInfo view_ci { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    view_ci.image = texture_slot.image;
    view_ci.viewType = texture_slot.size.is_2d() ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D;
    view_ci.format = translate::texture_format(texture_slot.format);
    view_ci.subresourceRange = sub_range;

    /* Create the image view */
    if (vkCreateImageView(gpu->logical_device, &view_ci, nullptr, &resource.data.view) != VK_SUCCESS) {
        return Err("failed to create image view.");
    }
    return Ok(resource.handle);
}

Result<Sampler> VRAMBank::create_sampler(Filter filter, AddressMode mode, BorderColor border) {
    /* Translate the filter, address mode, and border color */
    const VkFilter filter_mode = translate::sampler_filter(filter);
    const VkSamplerAddressMode address_mode = translate::sampler_address_mode(mode);
    const VkBorderColor border_color = translate::sampler_border_color(border);

    /* Pop a new sampler off the stock */
    StockPair resource = samplers.pop();

    /* Sampler creation info */
    VkSamplerCreateInfo sampler_ci { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler_ci.magFilter = filter_mode;
    sampler_ci.minFilter = filter_mode;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = address_mode;
    sampler_ci.addressModeV = address_mode;
    sampler_ci.addressModeW = address_mode;
    sampler_ci.borderColor = border_color;

    /* Create the sampler */
    if (vkCreateSampler(gpu->logical_device, &sampler_ci, nullptr, &resource.data.sampler) != VK_SUCCESS) {
        return Err("failed to create sampler.");
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
    swapchain_ci.presentMode = VK_PRESENT_MODE_MAILBOX_KHR; /* TODO: Vsync parameter. */
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

Result<void> VRAMBank::upload_buffer(Buffer& buffer, const void* data, u64 dst_offset, u64 size) {
    if (size == 0u) return Err("size is 0.");

    BufferSlot& slot = buffers.get(buffer);
    slot.size = size;

    if (has_flag(slot.usage, BufferUsage::TransferDst) == false) {
        gpu->log(DebugSeverity::Warning, "attempted to upload to buffer without TransferDst flag.");
        return Ok();
    }

    /* Create staging buffer */
    VkBufferCreateInfo staging_buffer_ci { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    staging_buffer_ci.size = size;
    staging_buffer_ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    staging_buffer_ci.queueFamilyIndexCount = 1u;
    staging_buffer_ci.pQueueFamilyIndices = &gpu->queue_families.queue_combined;

    /* Staging memory allocation info */
    VmaAllocationCreateInfo alloc_ci {};
    alloc_ci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;

    /* Create the staging buffer & allocate it using VMA */
    VkBuffer staging_buffer {};
    VmaAllocation alloc {};
    if (vmaCreateBuffer(vma_allocator, &staging_buffer_ci, &alloc_ci, &staging_buffer, &alloc, nullptr) != VK_SUCCESS) return Err("failed to create staging buffer.");
    memcpy(alloc->GetMappedData(), data, size);

    VkBufferCopy copy {};
    copy.srcOffset = 0u;
    copy.dstOffset = dst_offset;
    copy.size = size;

    if (begin_upload() == false) return Err("failed to begin upload."); /* Begin recording commands */
    vkCmdCopyBuffer(upload_cmd, staging_buffer, slot.buffer, 1u, &copy);
    if (end_upload() == false) return Err("failed to end upload."); /* End recording commands */

    /* Destroy staging buffer */
    vmaDestroyBuffer(vma_allocator, staging_buffer, alloc);
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

void VRAMBank::destroy_buffer(Buffer& buffer) {
    BufferSlot& slot = buffers.push(buffer);
    vmaDestroyBuffer(vma_allocator, slot.buffer, slot.alloc);
}

void VRAMBank::destroy_texture(Texture& texture) { 
    TextureSlot& slot = textures.push(texture);
    vmaDestroyImage(vma_allocator, slot.image, slot.alloc);
}

void VRAMBank::destroy_image(Image& image) { 
    ImageSlot& slot = images.push(image);
    vkDestroyImageView(gpu->logical_device, slot.view, nullptr);
}

void VRAMBank::destroy_sampler(Sampler& sampler) { 
    SamplerSlot& slot = samplers.push(sampler);
    vkDestroySampler(gpu->logical_device, slot.sampler, nullptr);
}
