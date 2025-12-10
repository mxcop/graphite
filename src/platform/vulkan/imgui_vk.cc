#include "imgui_vk.hh"

#ifdef GRAPHITE_IMGUI

#include <imgui_impl_vulkan.h> /* Vulkan impl for ImGUI */

#include "graphite/gpu_adapter.hh"
#include "graphite/vram_bank.hh"

/* ImGUI descriptor pool sizes. */
const VkDescriptorPoolSize DESC_POOL_SIZES[] {
    {VK_DESCRIPTOR_TYPE_SAMPLER, 256u},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256u},
    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256u},
    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 256u},
    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 256u},
    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 256u},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 256u},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 256u},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 256u},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 256u},
    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 256u}
};

Result<void> ImGUI::init(GPUAdapter &gpu, RenderTarget rt) {
    this->gpu = &gpu;

    /* Allocate the imgui descriptor pool */
    VkDescriptorPoolCreateInfo pool_ci { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_ci.maxSets = 128u;
    pool_ci.poolSizeCount = sizeof(DESC_POOL_SIZES) / sizeof(VkDescriptorPoolSize);
    pool_ci.pPoolSizes = DESC_POOL_SIZES;
    if (vkCreateDescriptorPool(gpu.logical_device, &pool_ci, nullptr, &desc_pool) != VK_SUCCESS) {
        return Err("failed to create imgui descriptor pool.");
    }

    /* Get the render target information */
    const RenderTargetSlot& rt_data = gpu.get_vram_bank().render_targets.get(rt);

    /* Initialize imgui */
    ImGui_ImplVulkan_InitInfo init_info {};
    init_info.ApiVersion = VK_API_VERSION;
    init_info.Instance = gpu.instance;
    init_info.PhysicalDevice = gpu.physical_device;
    init_info.Device = gpu.logical_device;
    init_info.QueueFamily = gpu.queue_families.queue_combined;
    init_info.Queue = gpu.queues.queue_combined;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = desc_pool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;
    init_info.PipelineInfoMain.RenderPass = VK_NULL_HANDLE;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &rt_data.format;

    /* Initialize ImGUI */
    if (!ImGui_ImplVulkan_Init(&init_info)) {
        return Err("failed to init imgui for vulkan.");
    }

    /* Bilinear sampler creation info */
    VkSamplerCreateInfo sampler_ci { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler_ci.magFilter = VK_FILTER_LINEAR;
    sampler_ci.minFilter = VK_FILTER_LINEAR;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

    /* Create a bilinear sampler for imgui */
    if (vkCreateSampler(gpu.logical_device, &sampler_ci, nullptr, &bilinear_sampler) != VK_SUCCESS) { 
        return Err("failed to create bilinear sampler for imgui.");
    }
    return Ok();
}

void ImGUI::new_frame() {
    ImGui_ImplVulkan_NewFrame();
}

u64 ImGUI::add_image(Image image) {
    const ImageSlot& image_data = gpu->get_vram_bank().images.get(image);
    const VkDescriptorSet handle = ImGui_ImplVulkan_AddTexture(bilinear_sampler, image_data.view, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

    image_map[image.raw()] = (u64&)handle;
    return (u64&)handle;
}

void ImGUI::remove_image(Image image) {
    if (image_map.count(image.raw()) == 0) return;
    ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet&)image_map[image.raw()]);
    image_map.erase(image.raw());
}

void ImGUI::render(VkCommandBuffer cmd) {
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data == nullptr) return;
    ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);
}

Result<void> ImGUI::deinit() {
    vkDeviceWaitIdle(gpu->logical_device);
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(gpu->logical_device, desc_pool, nullptr);
    vkDestroySampler(gpu->logical_device, bilinear_sampler, nullptr);
    return Ok();
}

#endif
