#include "imgui_vk.hh"

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

/* ImGUI vulkan pipeline info */
struct ImGui_ImplVulkan_PipelineInfo {
    VkRenderPass RenderPass;
    uint32_t Subpass; 
    VkSampleCountFlagBits MSAASamples = {};
    VkPipelineRenderingCreateInfoKHR PipelineRenderingCreateInfo;
};

/* ImGUI vulkan init info */
struct ImGui_ImplVulkan_InitInfo {
    uint32_t ApiVersion;
    VkInstance Instance;
    VkPhysicalDevice PhysicalDevice;
    VkDevice Device;
    uint32_t QueueFamily;
    VkQueue Queue;
    VkDescriptorPool DescriptorPool;     
    uint32_t DescriptorPoolSize;    
    uint32_t MinImageCount;      
    uint32_t ImageCount;      
    VkPipelineCache PipelineCache; 
    ImGui_ImplVulkan_PipelineInfo PipelineInfoMain;
    bool UseDynamicRendering;
    const VkAllocationCallbacks* Allocator;
    void (*CheckVkResultFn)(VkResult err);
    VkDeviceSize MinAllocationSize;
    VkShaderModuleCreateInfo CustomShaderVertCreateInfo;
    VkShaderModuleCreateInfo CustomShaderFragCreateInfo;
};

/* ImGUI graphics init function. */
bool ImGui_ImplGraphics_Init(ImGUIFunctions functions, ImGui_ImplVulkan_InitInfo* init_info) {
    return reinterpret_cast<bool(*)(void*)>(functions.graphics_init)(init_info);
}

/* ImGUI add texture function. */
VkDescriptorSet ImGui_ImplGraphics_AddTexture(ImGUIFunctions functions, VkSampler sampler, VkImageView image_view, VkImageLayout image_layout) {
    return reinterpret_cast<VkDescriptorSet(*)(VkSampler, VkImageView, VkImageLayout)>(functions.add_texture)(sampler, image_view, image_layout);
}

/* ImGUI remove texture function. */
void ImGui_ImplGraphics_RemoveTexture(ImGUIFunctions functions, VkDescriptorSet desc_set) {
    reinterpret_cast<void(*)(VkDescriptorSet)>(functions.remove_texture)(desc_set);
}

/* ImGUI graphics new frame function. */
void ImGui_ImplGraphics_NewFrame(ImGUIFunctions functions) {
    reinterpret_cast<void(*)()>(functions.new_frame)();
}

/* ImGUI get draw data function. */
void* ImGui_GetDrawData(ImGUIFunctions functions) {
    return reinterpret_cast<void*(*)()>(functions.draw_data)();
}

/* ImGUI graphics render draw data function. */
void ImGui_ImplGraphics_RenderDrawData(ImGUIFunctions functions, void* draw_data, VkCommandBuffer cmd) {
    reinterpret_cast<void(*)(void*, VkCommandBuffer, void*)>(functions.render)(draw_data, cmd, nullptr);
}

/* ImGUI graphics shutdown function. */
void ImGui_ImplGraphics_Shutdown(ImGUIFunctions functions) {
    reinterpret_cast<void(*)()>(functions.graphics_shutdown)();
}

Result<void> ImGUI::init(GPUAdapter &gpu, RenderTarget rt, ImGUIFunctions functions) {
    this->gpu = &gpu;
    this->functions = functions;

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
    if (!ImGui_ImplGraphics_Init(functions, &init_info)) {
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
    ImGui_ImplGraphics_NewFrame(functions);
}

u64 ImGUI::add_texture(Texture texture) {
    // gpu->get_vram_bank().get_texture(texture);
    // TODO: Get image view and image layout for texture here.
    // return ImGui_ImplGraphics_AddTexture(functions, bilinear_sampler, );
    return 0;
}

void ImGUI::remove_texture(Texture texture) {
    // TODO: Remove texture from ImGUI and remove it from the std::unordered_map
}

void ImGUI::render(VkCommandBuffer cmd) {
    void* draw_data = ImGui_GetDrawData(functions);
    if (draw_data == nullptr) return;
    ImGui_ImplGraphics_RenderDrawData(functions, draw_data, cmd);
}

Result<void> ImGUI::destroy() {
    vkDeviceWaitIdle(gpu->logical_device);
    ImGui_ImplGraphics_Shutdown(functions);
    vkDestroyDescriptorPool(gpu->logical_device, desc_pool, nullptr);
    vkDestroySampler(gpu->logical_device, bilinear_sampler, nullptr);
    return Ok();
}
