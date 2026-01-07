#include "vram_bank_vk.hh"

#include <utility>

#include "wrapper/translate_vk.hh"

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
            return Err("failed to import vulkan functions from volk (required for vma init).");

        vma_info.pVulkanFunctions = &vulkan_functions;

        if (vmaCreateAllocator(&vma_info, &vma_allocator) != VK_SUCCESS)
            return Err("failed to initialise vma.");
    }

    /* Initialize the Stack Pools */
    render_targets.init(gpu.get_max_render_targets());
    buffers.init(gpu.get_max_buffers());
    textures.init(gpu.get_max_textures());
    images.init(gpu.get_max_images());
    samplers.init(gpu.get_max_samplers());

    { /* Init Bindless */
        /* Initialize the bindless resources */
        const VkDescriptorPoolSize bindless_pool_sizes[] {
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, images.stack_size},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, buffers.stack_size}
        };

        /* Allocate the bindless descriptor pool */
        VkDescriptorPoolCreateInfo bindless_pool_ci { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        bindless_pool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        bindless_pool_ci.maxSets = 1u;
        bindless_pool_ci.poolSizeCount = sizeof(bindless_pool_sizes) / sizeof(VkDescriptorPoolSize);
        bindless_pool_ci.pPoolSizes = bindless_pool_sizes;
        if (vkCreateDescriptorPool(gpu.logical_device, &bindless_pool_ci, nullptr, &bindless_pool) != VK_SUCCESS) {
            return Err("failed to create bindless descriptor pool.");
        }
        gpu.set_object_name(VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_POOL, (u64)bindless_pool, "Bindless Descriptor Pool");

        /* Create the bindless descriptor set layout */
        const VkDescriptorBindingFlags flags[] {
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
        };

        VkDescriptorSetLayoutBindingFlagsCreateInfo bindless_flags { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
        bindless_flags.bindingCount = 2u;
        bindless_flags.pBindingFlags = flags;

        VkDescriptorSetLayoutBinding bindless_bindings[2u] {};
        bindless_bindings[0].binding = BINDLESS_TEXTURE_SLOT;
        bindless_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindless_bindings[0].descriptorCount = images.stack_size;
        bindless_bindings[0].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;

        bindless_bindings[1].binding = BINDLESS_BUFFER_SLOT;
        bindless_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindless_bindings[1].descriptorCount = buffers.stack_size;
        bindless_bindings[1].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo bindless_set_ci { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        bindless_set_ci.pNext = &bindless_flags;
        bindless_set_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        bindless_set_ci.bindingCount = sizeof(bindless_bindings) / sizeof(VkDescriptorSetLayoutBinding);
        bindless_set_ci.pBindings = bindless_bindings;

        if (vkCreateDescriptorSetLayout(gpu.logical_device, &bindless_set_ci, nullptr, &bindless_layout) != VK_SUCCESS) {
            return Err("failed to create bindless descriptor layout.");
        }
        gpu.set_object_name(VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)bindless_layout, "Bindless Descriptor Set Layout");

        /* Allocate the bindless descriptor set */
        VkDescriptorSetAllocateInfo bindless_set_ai { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        bindless_set_ai.descriptorPool = bindless_pool;
        bindless_set_ai.descriptorSetCount = 1u;
        bindless_set_ai.pSetLayouts = &bindless_layout;

        if (vkAllocateDescriptorSets(gpu.logical_device, &bindless_set_ai, &bindless_set) != VK_SUCCESS) {
            return Err("failed to create bindless descriptor set.");
        }
        gpu.set_object_name(VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)bindless_set, "Bindless Descriptor Set");
    }

    /* Command pool creation info */
    VkCommandPoolCreateInfo pool_ci { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_ci.queueFamilyIndex = gpu.queue_families.queue_transfer;

    /* Create command pool for upload command buffer */
    if (vkCreateCommandPool(gpu.logical_device, &pool_ci, nullptr, &upload_cmd_pool) != VK_SUCCESS) {
        return Err("failed to create command pool for render graph.");
    }
    gpu.set_object_name(VkObjectType::VK_OBJECT_TYPE_COMMAND_POOL, (u64)upload_cmd_pool, "Upload Command Pool");

    /* Allocate upload command buffer */
    VkCommandBufferAllocateInfo cmd_ai { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmd_ai.commandPool = upload_cmd_pool;
    cmd_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_ai.commandBufferCount = 1u;

    if (vkAllocateCommandBuffers(gpu.logical_device, &cmd_ai, &upload_cmd) != VK_SUCCESS) {
        return Err("failed to create upload command buffer.");
    }
    gpu.set_object_name(VkObjectType::VK_OBJECT_TYPE_COMMAND_BUFFER, (u64)upload_cmd, "Upload Command Buffer");

    /* Create the upload fence */
    VkFenceCreateInfo fence_ci { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

    if (vkCreateFence(gpu.logical_device, &fence_ci, nullptr, &upload_fence) != VK_SUCCESS) {
        return Err("failed to create upload fence");
    }
    gpu.set_object_name(VkObjectType::VK_OBJECT_TYPE_FENCE, (u64)upload_fence, "Upload Fence");

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

Result<void> VRAMBank::deinit() {
    vkDestroyDescriptorPool(gpu->logical_device, bindless_pool, nullptr);
    vkDestroyDescriptorSetLayout(gpu->logical_device, bindless_layout, nullptr);

    vkDestroyCommandPool(gpu->logical_device, upload_cmd_pool, nullptr);

    vkDestroyFence(gpu->logical_device, upload_fence, nullptr);

    vmaDestroyAllocator(vma_allocator);
    return Ok();
}

Result<RenderTarget> VRAMBank::create_render_target(const TargetDesc& target, bool vsync, u32 width, u32 height) {
    /* Pop a new render target off the stock */
    StockPair resource = render_targets.pop();

    /* Create a KHR surface */
    VkWin32SurfaceCreateInfoKHR surface_ci { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    surface_ci.hwnd = target.window;

    if (vkCreateWin32SurfaceKHR(gpu->instance, &surface_ci, nullptr, &resource.data.surface) != VK_SUCCESS) {
        return Err("failed to create win32 surface.");
    }
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_SURFACE_KHR, (u64)resource.data.surface, "Win32 Surface");

    /* Get the capabilities of the KHR surface */
    VkSurfaceCapabilitiesKHR surface_features {};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->physical_device, resource.data.surface, &surface_features) != VK_SUCCESS) {
        return Err("failed to get surface capabilities.");
    }

    /* Use up to 4 images that are available, this allows for triple buffering */
    resource.data.image_count = std::min(4u, std::max(1u, surface_features.maxImageCount));

    /* Set the width and height, with respect to the surface limits */
    resource.data.extent.width = std::min(std::max(width, surface_features.minImageExtent.width), surface_features.maxImageExtent.width);
    resource.data.extent.height = std::min(std::max(width, surface_features.minImageExtent.height), surface_features.maxImageExtent.height);

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

    /* Get the available surface presentation modes */
    u32 present_mode_count = 0u;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu->physical_device, resource.data.surface, &present_mode_count, nullptr);
    VkPresentModeKHR* present_modes = new VkPresentModeKHR[present_mode_count] {};
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu->physical_device, resource.data.surface, &present_mode_count, present_modes);

    /* Find the presentation mode we want */
    resource.data.present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    for (u32 i = 0u; i < present_mode_count; ++i) {
        /* Mailbox is the preferred vsync present mode */
        if (vsync == true && present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            resource.data.present_mode = present_modes[i];
            break;
        }
        /* FIFO is the back-up vsync present mode */
        if (vsync == true && present_modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
            resource.data.present_mode = present_modes[i];
        }
        /* Immediate is the preferred non-vsync present mode */
        if (vsync == false && present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            resource.data.present_mode = present_modes[i];
            break;
        }
    }
    delete[] present_modes; /* Free the present modes */

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
    swapchain_ci.presentMode = resource.data.present_mode;
    swapchain_ci.clipped = true;

    /* Create the swapchain */
    if (vkCreateSwapchainKHR(gpu->logical_device, &swapchain_ci, nullptr, &resource.data.swapchain) != VK_SUCCESS) {
        return Err("failed to create swapchain for render target.");
    }
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_SWAPCHAIN_KHR, (u64)resource.data.swapchain, "Swapchain");

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
        const std::string image_view_name = "Swapchain Image View #" + std::to_string(i);
        gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_IMAGE_VIEW, (u64)resource.data.views[i], image_view_name.c_str());

        /* Create image presentation semaphore */
        if (vkCreateSemaphore(gpu->logical_device, &sema_ci, nullptr, &resource.data.semaphores[i]) != VK_SUCCESS) {
            return Err("failed to create semaphore for render target.");
        }
        const std::string semaphore_name = "Swapchain Image Semaphore #" + std::to_string(i);
        gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_SEMAPHORE, (u64)resource.data.semaphores[i], semaphore_name.c_str());
    }

    return Ok(resource.handle);
}

Result<Buffer> VRAMBank::create_buffer(std::string name, BufferUsage usage, u64 count, u64 stride) {
    /* Make sure the buffer usage is valid */
    if (usage == BufferUsage::Invalid) return Err("invalid buffer usage.");

    /* Pop a new buffer off the stock */
    StockPair resource = buffers.pop();
    resource.data.usage = usage;

    /* Size of the buffer in bytes */
    const u64 size = stride == 0 ? count : count * stride;
    resource.data.size = size;

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
    if (name.empty()) name = "Buffer #" + std::to_string(resource.handle.get_index());
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_BUFFER, (u64)resource.data.buffer, name.c_str());
    resource.data.name = name;

    /* Insert only Storage Buffers in the bindless descriptor set */
    if (has_flag(usage, BufferUsage::Storage)) {
        /* Create the bindless descriptor write template */
        VkDescriptorBufferInfo buffer_info {};
        buffer_info.buffer = resource.data.buffer;
        buffer_info.offset = 0u;
        buffer_info.range = size;

        VkWriteDescriptorSet bindless_write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        bindless_write.dstSet = bindless_set;
        bindless_write.dstArrayElement = resource.handle.get_index() - 1u;
        bindless_write.descriptorCount = 1u;
        bindless_write.pBufferInfo = &buffer_info;
        bindless_write.dstBinding = BINDLESS_BUFFER_SLOT;
        bindless_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

        vkUpdateDescriptorSets(gpu->logical_device, 1u, &bindless_write, 0u, nullptr);
    }

    return Ok(resource.handle);
}

Result<Texture> VRAMBank::create_texture(std::string name, TextureUsage usage, TextureFormat fmt, Size3D size, TextureMeta meta) {
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
    texture_ci.extent = { std::max(size.x, 1u), std::max(size.y, 1u), std::max(size.z, 1u) };
    texture_ci.mipLevels = std::max(1u, meta.mips);
    texture_ci.arrayLayers = std::max(1u, meta.arrays);
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
    if (name.empty()) name = "Texture #" + std::to_string(resource.handle.get_index());
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_IMAGE, (u64)resource.data.image, name.c_str());
    resource.data.name = name;

    return Ok(resource.handle);
}

Result<Image> VRAMBank::create_image(std::string name, Texture texture, u32 mip, u32 layer) {
    /* Make sure the texture is valid */
    if (texture.is_null()) return Err("cannot create image for texture which is null.");

    /* Pop a new image off the stock */
    StockPair resource = images.pop();
    resource.data.texture = texture;
    TextureSlot& texture_slot = textures.get(texture);
    texture_slot.images.push_back(resource.handle);
    
    /* Image access sub resource range */
    VkImageSubresourceRange sub_range {};
    sub_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; /* Color hardcoded! (might want depth too) */
    sub_range.baseMipLevel = mip;
    sub_range.levelCount = std::max(1u, texture_slot.meta.mips - mip);
    sub_range.baseArrayLayer = layer;
    sub_range.layerCount = std::max(1u, texture_slot.meta.arrays - layer);
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
    if (name.empty()) name = "Image #" + std::to_string(resource.handle.get_index());
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_IMAGE_VIEW, (u64)resource.data.view, name.c_str());
    resource.data.name = name;

    /* Insert readonly images into the bindless descriptor set */
    if (has_flag(texture_slot.usage, TextureUsage::Sampled)) { 
        /* Create the bindless descriptor write template */
        VkDescriptorImageInfo image_info {};
        image_info.sampler = VK_NULL_HANDLE;
        image_info.imageView = resource.data.view;
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet bindless_write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        bindless_write.dstSet = bindless_set;
        bindless_write.dstArrayElement = resource.handle.get_index() - 1u;
        bindless_write.descriptorCount = 1u;
        bindless_write.pImageInfo = &image_info;
        bindless_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindless_write.dstBinding = BINDLESS_TEXTURE_SLOT;

        vkUpdateDescriptorSets(gpu->logical_device, 1u, &bindless_write, 0u, nullptr);
    }

    return Ok(resource.handle);
}

Result<Sampler> VRAMBank::create_sampler(std::string name, Filter filter, AddressMode mode, BorderColor border) {
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
    if (name.empty()) name = "Sampler #" + std::to_string(resource.handle.get_index());
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_SAMPLER, (u64)resource.data.sampler, name.c_str());
    resource.data.name = name;

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
    data.extent.width = std::min(std::max(width, surface_features.minImageExtent.width), surface_features.maxImageExtent.width);
    data.extent.height = std::min(std::max(width, surface_features.minImageExtent.height), surface_features.maxImageExtent.height);

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
    swapchain_ci.presentMode = data.present_mode;
    swapchain_ci.clipped = true;
    swapchain_ci.oldSwapchain = data.swapchain;

    /* Create the swapchain */
    if (vkCreateSwapchainKHR(gpu->logical_device, &swapchain_ci, nullptr, &data.swapchain) != VK_SUCCESS) {
        return Err("failed to re-create swapchain for render target.");
    }
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_SWAPCHAIN_KHR, (u64)data.swapchain, "Win32 Surface");

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
        const std::string image_view_name = "Swapchain Image View #" + std::to_string(i);
        gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_IMAGE_VIEW, (u64)data.views[i], image_view_name.c_str());
    }

    /* Destroy the old swapchain */
    vkDestroySwapchainKHR(gpu->logical_device, swapchain_ci.oldSwapchain, nullptr);

    return Ok();
}

Result<void> VRAMBank::resize_texture(Texture& texture, Size3D size) {
    /* Wait for the device to idle */
    vkQueueWaitIdle(gpu->queues.queue_combined);

    size.x = std::max(1u, size.x);
    size.y = std::max(1u, size.y);

    if (size.x > 8192 || size.y > 8192) {
        return Err("texture size was larger 8192.");
    }

    TextureSlot& data = textures.get(texture);
    data.size = size;
    data.layout = VK_IMAGE_LAYOUT_UNDEFINED;

    /* Destroy All Image Views */
    for (u32 i = 0; i < data.images.size(); i++) {
        ImageSlot& image = images.get(data.images[i]);
        vkDestroyImageView(gpu->logical_device, image.view, nullptr);
    }

    /* Destroy Image */
    vmaDestroyImage(vma_allocator, data.image, data.alloc);

    VkFormat format = translate::texture_format(data.format);

    /* Image creation info */
    VkImageCreateInfo texture_ci {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    texture_ci.imageType = size.is_2d() ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
    texture_ci.format = format;
    texture_ci.extent = {std::max(size.x, 1u), std::max(size.y, 1u), std::max(size.z, 1u)};
    texture_ci.mipLevels = std::max(1u, data.meta.mips);
    texture_ci.arrayLayers = std::max(1u, data.meta.arrays);
    texture_ci.samples = VK_SAMPLE_COUNT_1_BIT; /* No MSAA */
    texture_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    texture_ci.usage = translate::texture_usage(data.usage);
    texture_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    texture_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    /* Memory allocation info */
    VmaAllocationCreateInfo alloc_ci {};
    alloc_ci.flags = 0x00u;
    alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;

    /* Create the texture & allocate it using VMA */
    if (vmaCreateImage(vma_allocator, &texture_ci, &alloc_ci, &data.image, &data.alloc, nullptr) !=
        VK_SUCCESS) {
        return Err("failed to allocate image resource.");
    }
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_IMAGE, (u64)data.image, data.name.c_str());

    /* Re-create Image Views */
    for (u32 i = 0; i < data.images.size(); i++) {
        ImageSlot& image = images.get(data.images[i]);

        /* Image view creation info */
        VkImageViewCreateInfo view_ci {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_ci.image = data.image;
        view_ci.viewType = size.is_2d() ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D;
        view_ci.format = translate::texture_format(data.format);
        view_ci.subresourceRange = image.sub_range;

        /* Create the image view */
        if (vkCreateImageView(gpu->logical_device, &view_ci, nullptr, &image.view) != VK_SUCCESS) {
            return Err("failed to create image view.");
        }
        gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_IMAGE_VIEW, (u64)image.view, image.name.c_str());

        /* Insert readonly images into the bindless descriptor set */
        if (has_flag(data.usage, TextureUsage::Sampled)) {
            /* Create the bindless descriptor write template */
            VkDescriptorImageInfo image_info {};
            image_info.sampler = VK_NULL_HANDLE;
            image_info.imageView = image.view;
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet bindless_write {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            bindless_write.dstSet = bindless_set;
            bindless_write.dstArrayElement = data.images[i].get_index() - 1u;
            bindless_write.descriptorCount = 1u;
            bindless_write.pImageInfo = &image_info;
            bindless_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            bindless_write.dstBinding = BINDLESS_TEXTURE_SLOT;

            vkUpdateDescriptorSets(gpu->logical_device, 1u, &bindless_write, 0u, nullptr);
        }
    }

    return Ok();
}

Result<void> VRAMBank::resize_buffer(Buffer& buffer, u64 count, u64 stride) {
    /* Wait for the device to idle */
    vkQueueWaitIdle(gpu->queues.queue_combined);

    /* Get the buffer resource slot */
    BufferSlot& data = buffers.get(buffer);

    /* Size of the buffer in bytes */
    const u64 size = stride == 0 ? count : count * stride;
    data.size = size;

    /* Destroy the existing buffer */
    vmaDestroyBuffer(vma_allocator, data.buffer, data.alloc);

    /* Buffer creation info */
    VkBufferCreateInfo buffer_ci { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_ci.size = size;
    buffer_ci.usage = translate::buffer_usage(data.usage);
    buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_ci.queueFamilyIndexCount = 1u;
    buffer_ci.pQueueFamilyIndices = &gpu->queue_families.queue_combined;

    /* Memory allocation info */
    VmaAllocationCreateInfo alloc_ci {};
    alloc_ci.flags = 0x00u;
    alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;

    /* Create the buffer & allocate it using VMA */
    if (vmaCreateBuffer(vma_allocator, &buffer_ci, &alloc_ci, &data.buffer, &data.alloc, nullptr) != VK_SUCCESS) { 
        return Err("failed to create buffer.");
    }
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_BUFFER, (u64)data.buffer, data.name.c_str());

    /* Insert only Storage Buffers in the bindless descriptor set */
    if (has_flag(data.usage, BufferUsage::Storage)) {
        /* Create the bindless descriptor write template */
        VkDescriptorBufferInfo buffer_info {};
        buffer_info.buffer = data.buffer;
        buffer_info.offset = 0u;
        buffer_info.range = size;

        VkWriteDescriptorSet bindless_write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        bindless_write.dstSet = bindless_set;
        bindless_write.dstArrayElement = buffer.get_index() - 1u;
        bindless_write.descriptorCount = 1u;
        bindless_write.pBufferInfo = &buffer_info;
        bindless_write.dstBinding = BINDLESS_BUFFER_SLOT;
        bindless_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

        vkUpdateDescriptorSets(gpu->logical_device, 1u, &bindless_write, 0u, nullptr);
    }

    return Ok();
}

Result<void> VRAMBank::upload_buffer(Buffer& buffer, const void* data, u64 dst_offset, u64 size) {
    if (size == 0u) return Err("size is 0.");

    BufferSlot& slot = buffers.get(buffer);
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

    /* Copy data into the staging buffer */
    void* staging_memory = nullptr;
    vmaMapMemory(vma_allocator, alloc, &staging_memory);
    memcpy(staging_memory, data, size); 
    vmaUnmapMemory(vma_allocator, alloc);

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

Result<void> VRAMBank::upload_texture(Texture& texture, const void* data, const u64 size) {
    /* Make sure the texture can be transfered to */
    TextureSlot& texture_slot = textures.get(texture);
    if (has_flag(texture_slot.usage, TextureUsage::TransferDst) == false) return Err("the texture flags don't support transferring to.");

    /* Staging buffer creation info */
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
    VkBuffer staging_buffer;
    VmaAllocation alloc;
    if (vmaCreateBuffer(vma_allocator, &staging_buffer_ci, &alloc_ci, &staging_buffer, &alloc, nullptr) != VK_SUCCESS) return Err("failed to create staging buffer");

    /* Copy data into the staging buffer */
    void* staging_memory = nullptr;
    vmaMapMemory(vma_allocator, alloc, &staging_memory);
    memcpy(staging_memory, data, size); 
    vmaUnmapMemory(vma_allocator, alloc);

    VkBufferImageCopy copy {};
    copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u };
    copy.imageExtent = VkExtent3D { std::max(texture_slot.size.x, 1u), std::max(texture_slot.size.y, 1u), std::max(texture_slot.size.z, 1u) };

    /* Create an image layout transition barrier */
    VkImageMemoryBarrier2 image_barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    image_barrier.srcAccessMask = VK_ACCESS_2_NONE;
    image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_2_NONE;
    image_barrier.oldLayout = texture_slot.layout;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_barrier.image = texture_slot.image;
    image_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u };
    texture_slot.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    /* Render target dependency info */
    VkDependencyInfo dep_info { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dep_info.imageMemoryBarrierCount = 1u;
    dep_info.pImageMemoryBarriers = &image_barrier;

    if (begin_upload() == false) return Err("failed to begin upload."); /* Begin recording commands */
    vkCmdPipelineBarrier2KHR(upload_cmd, &dep_info);
    vkCmdCopyBufferToImage(upload_cmd, staging_buffer, texture_slot.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copy);
    if (end_upload() == false) return Err("failed to end upload."); /* End recording commands */

    vmaDestroyBuffer(vma_allocator, staging_buffer, alloc); /* Destroy staging buffer */

    return Ok();
}

Texture VRAMBank::get_texture(Image image) { 
    return images.get(image).texture; 
}

void VRAMBank::destroy_render_target(RenderTarget &render_target) {
    /* For render targets we need to wait for queue idle */
    vkQueueWaitIdle(gpu->queues.queue_combined);
    
    /* Push the handle back onto the stock, and get its slot for cleanup */
    RenderTargetSlot& slot = render_targets.push(render_target);

    /* Destroy the images, layouts, views, & semaphores */
    delete[] slot.images;
    delete[] slot.old_layouts;
    for (u32 i = 0u; i < slot.image_count; ++i) {
        vkDestroyImageView(gpu->logical_device, slot.views[i], nullptr);
        vkDestroySemaphore(gpu->logical_device, slot.semaphores[i], nullptr);
    }
    delete[] slot.views;
    delete[] slot.semaphores;

    /* Destroy the swapchain & surface */
    vkDestroySwapchainKHR(gpu->logical_device, slot.swapchain, nullptr);
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
