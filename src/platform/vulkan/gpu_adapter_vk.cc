#include "gpu_adapter_vk.hh"

#include "wrapper/extensions_vk.hh"
#include "wrapper/device_selection_vk.hh"

/* Windowing instance extension for the current build platform */
#if defined(_WIN32) || defined(_WIN64)
const char* WINDOWING_EXTENSION = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#else
#error "TODO: provide windowing instance extension for this platform!"
#endif

/* Validation layer to use for debugging */
const char* VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";

/* Custom vulkan debug msg callback */
VkBool32 vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* cb_data, void* data);

/* Initialize the GPU adapter. */
Result<void> GPUAdapter::init(bool debug_mode) {
    /* Load Vulkan API functions */
    if (volkInitialize() != VK_SUCCESS) return Err("failed to initialize volk. (vulkan meta loader)");

    /* Check if debugging & validation is supported */
    validation = debug_mode && query_debug_support(VALIDATION_LAYER) && query_validation_support(VALIDATION_LAYER);
    if (debug_mode && validation == false) {
        this->log(DebugSeverity::Warning, "validation layers were requested, but are not supported.");
    }

    /* Vulkan app creation info */
    VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName = "graphite";
    app_info.pEngineName = "graphite";
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    /* Debug utilities messenger (for debug mode) */
    VkDebugUtilsMessengerCreateInfoEXT debug_utils { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debug_utils.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_utils.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_utils.pUserData = (void*)&logger;
    debug_utils.pfnUserCallback = vk_debug_callback;

    /* Vulkan instance extensions & layers */
    const uint32_t instance_ext_count = validation ? 3u : 2u;
    const char* const instance_ext[3] = {VK_KHR_SURFACE_EXTENSION_NAME, WINDOWING_EXTENSION, VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    const uint32_t instance_layers_count = validation ? 1u : 0u;
    const char* const instance_layers[1] = {VALIDATION_LAYER};

    /* Check if all required instance extensions are supported */
    if (const Result r = query_instance_support(instance_ext, instance_ext_count); r.is_err()) {
        return Err(r.unwrap_err().c_str());
    }

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

    /* Select a suitable physical device */
    if (const Result r = select_physical_device(instance, device_ext, device_ext_count); r.is_ok()) {
        physical_device = r.unwrap();
    } else return Err(r.unwrap_err());

    /* Log the selected physical device name */
    this->log(DebugSeverity::Info, ("selected gpu: " + get_physical_device_name(physical_device)).c_str());
    
    /* TODO: Gather queue family indices we need using "vkGetPhysicalDeviceWin32PresentationSupportKHR" */

    /* TODO: Create the logical device */

    return Ok();
}

/* Destroy the GPU adapter, free all its resources. */
Result<void> GPUAdapter::destroy() {
    if (validation) { 
        vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);

    return Ok();
}

/* Vulkan validation layer callback function. */
VkBool32 vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* cb_data, void* data) {
    if (data == nullptr) return VK_FALSE;

    /* Convert the Vulkan severity to the platform agnostic debug severity */
    DebugSeverity debug_severity = DebugSeverity::Info;
    switch (severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: [[fallthrough]];
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        debug_severity = DebugSeverity::Info; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        debug_severity = DebugSeverity::Warning; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        debug_severity = DebugSeverity::Error; break;
    }

    /* Call the debug logger callback function */
    const DebugLogger* logger = (DebugLogger*)data;
    if (logger->logger == nullptr) return VK_FALSE;
    if (passes_level(logger->log_level, debug_severity) == false) return VK_FALSE;
    logger->logger(debug_severity, cb_data->pMessage, logger->user_data);

    return VK_FALSE;
}
