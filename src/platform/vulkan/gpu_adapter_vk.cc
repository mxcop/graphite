#include "gpu_adapter_vk.hh"

#include <cstdio>

const uint32_t instance_layers_count = 1u;
const char* const instance_layers[1] = {"VK_LAYER_KHRONOS_validation"};
const uint32_t instance_ext_count = 1u;
const char* const instance_ext[1] = {"VK_EXT_DEBUG_UTILS_EXTENSION_NAME"};

/* Custom vulkan debug msg callback */
inline VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* cb_data, void*) {
    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            printf("warn: %s\n", cb_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            printf("err: %s\n", cb_data->pMessage);
            break;
    }

    return VK_FALSE;
}

Result<void> GPUAdapter::init(bool debug_mode) {
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

    /* Vulkan instance creation info */
    VkInstanceCreateInfo instance_ci { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    if (debug_mode) {
        instance_ci.pNext = &debug_utils;
        instance_ci.enabledLayerCount = instance_layers_count;
        instance_ci.ppEnabledLayerNames = instance_layers;
        instance_ci.enabledExtensionCount = instance_ext_count;
        instance_ci.ppEnabledExtensionNames = instance_ext;
    }
    instance_ci.pApplicationInfo = &app_info;

    /* Load Vulkan API functions */
    if (volkInitialize() != VK_SUCCESS) return Err("failed to initialize volk.");

    /* Create a Vulkan instance */
    if (vkCreateInstance(&instance_ci, nullptr, &instance) != VK_SUCCESS) {
        return Err("failed to create vulkan instance.");
    }

    /* Load instance Vulkan API functions */
    volkLoadInstance(instance);

    if (debug_mode) {
        vkCreateDebugUtilsMessengerEXT(instance, &debug_utils, nullptr, &debug_messenger);
    }

    printf("created instance!\n");

    return Ok();
}
