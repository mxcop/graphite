#include "imgui_vk.hh"

// #define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
// #define IMGUI_IMPL_VULKAN_USE_VOLK
// #include <imgui_impl_vulkan.h>

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

struct ImGui_ImplVulkan_PipelineInfo {
    VkRenderPass                    RenderPass;
    uint32_t                        Subpass; 
    VkSampleCountFlagBits           MSAASamples = {};
    VkPipelineRenderingCreateInfoKHR PipelineRenderingCreateInfo;
};

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

Result<void> ImGUI::init(GPUAdapter& gpu, RenderTarget rt, ImGUIFunctions functions) {
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

    const RenderTargetSlot& rt_data = gpu.get_vram_bank().get_render_target(rt);

    /* Initialize imgui */
    ImGui_ImplVulkan_InitInfo init_info {};
    init_info.ApiVersion = VK_API_VERSION_1_2;
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

    bool (*ImGui_ImplGraphics_Init)(void*) = reinterpret_cast<bool(*)(void*)>(
        reinterpret_cast<uintptr_t>(functions.GraphicsInit)
    );

    if (!ImGui_ImplGraphics_Init(&init_info)) {
        return Err("failed to init imgui for vulkan.");
    }
    // if (!ImGui_ImplVulkan_CreateFontsTexture()) {
    //     return Err("failed to create fonts & textures for imgui.");
    // }

    // /* Create the sampler to be used for the imgui viewport */
    // const VkSamplerCreateInfo sampler_ci {
    //     .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    //     .magFilter = VK_FILTER_LINEAR,
    //     .minFilter = VK_FILTER_LINEAR,
    //     .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    //     .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    //     .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    //     .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    //     .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK};

    // /* Create the sampler */
    // if (vkCreateSampler(gpc->device, &sampler_ci, nullptr, &imgui_viewport_sampler) != VK_SUCCESS) return false;

    return Ok();
}

Result<void> ImGUI::destroy() {
    return Ok();
}
