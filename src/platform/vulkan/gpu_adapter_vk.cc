#include "gpu_adapter_vk.hh"

#include <cstdio>

#include "wrapper/extensions_vk.hh"

const char* VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";

/* Custom vulkan debug msg callback */
inline VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* cb_data, void*) {
    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            printf("[vulkan] warn: %s\n", cb_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            printf("[vulkan] err: %s\n", cb_data->pMessage);
            break;
    }

    return VK_FALSE;
}

Result<void> GPUAdapter::init(bool debug_mode) {
    /* Load Vulkan API functions */
    if (volkInitialize() != VK_SUCCESS) return Err("failed to initialize volk. (vulkan meta loader)");

    /* Check if debugging & validation is supported */
    validation = debug_mode && query_debug_support(VALIDATION_LAYER) && query_validation_support(VALIDATION_LAYER);
    if (debug_mode && validation == false) {
        printf("[vulkan] warn: validation layers were requested, but are not supported.\n");
    }

    /* Vulkan app creation info */
    VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName = "graphite";
    app_info.pEngineName = "graphite";
    app_info.apiVersion = VK_API_VERSION_1_2;
    
    /* TODO: Get required extensions from windowing library (e.g. SDL, GLFW) */

    /* Debug utilities messenger (for debug mode) */
    VkDebugUtilsMessengerCreateInfoEXT debug_utils { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debug_utils.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_utils.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_utils.pfnUserCallback = vk_debug_callback;

    /* Vulkan instance extensions & layers */
    const uint32_t instance_ext_count = validation ? 1u : 0u;
    const char* const instance_ext[1] = {"VK_EXT_DEBUG_UTILS_EXTENSION_NAME"};
    const uint32_t instance_layers_count = validation ? 1u : 0u;
    const char* const instance_layers[1] = {VALIDATION_LAYER};

    /* Vulkan instance creation info */
    VkInstanceCreateInfo instance_ci { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    if (validation) {
        instance_ci.pNext = &debug_utils;
        instance_ci.enabledLayerCount = instance_layers_count;
        instance_ci.ppEnabledLayerNames = instance_layers;
        instance_ci.enabledExtensionCount = instance_ext_count;
        instance_ci.ppEnabledExtensionNames = instance_ext;
    }
    instance_ci.pApplicationInfo = &app_info;

    /* Create a Vulkan instance */
    if (const VkResult r = vkCreateInstance(&instance_ci, nullptr, &instance); r != VK_SUCCESS) {
        if (r == VK_ERROR_LAYER_NOT_PRESENT) return Err("failed to create vulkan instance. (layer not supported)");
        if (r == VK_ERROR_EXTENSION_NOT_PRESENT) return Err("failed to create vulkan instance. (extension not supported)");
        return Err("failed to create vulkan instance.");
    }

    /* Load instance Vulkan API functions */
    volkLoadInstanceOnly(instance);

    /* Create a debug messenger if debug mode is turned on */
    if (validation) {
        vkCreateDebugUtilsMessengerEXT(instance, &debug_utils, nullptr, &debug_messenger);
    }

    /* TODO: get physical device (check extension support using "query_extension_support()") */

    printf("initialized gpu adapter!\n");
    return Ok();
}

Result<void> GPUAdapter::destroy() {
    if (validation) { 
        vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);

    printf("cleaned up gpu adapter!\n");
    return Ok();
}
