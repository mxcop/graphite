#include "gpu_adapter_vk.hh"

#include "graphite/vram_bank.hh"
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
    app_info.apiVersion = VK_API_VERSION;
    
    /* Debug utilities messenger (for debug mode) */
    VkDebugUtilsMessengerCreateInfoEXT debug_utils { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debug_utils.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_utils.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_utils.pUserData = (void*)&logger;
    debug_utils.pfnUserCallback = vk_debug_callback;

    /* Vulkan instance extensions & layers */
    const u32 instance_ext_count = validation ? 3u : 2u;
    const char* const instance_ext[3] = {VK_KHR_SURFACE_EXTENSION_NAME, WINDOWING_EXTENSION, VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    const u32 instance_layers_count = validation ? 1u : 0u;
    const char* const instance_layers[1] = {VALIDATION_LAYER};

    /* Check if all required instance extensions are supported */
    if (const Result r = query_instance_support(instance_ext, instance_ext_count); r.is_err()) {
        return Err(r.unwrap_err());
    }

    /* Vulkan instance creation info */
    VkInstanceCreateInfo instance_ci { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    if (validation) {
        instance_ci.pNext = &debug_utils;
        instance_ci.enabledLayerCount = instance_layers_count;
        instance_ci.ppEnabledLayerNames = instance_layers;
    }
    instance_ci.enabledExtensionCount = instance_ext_count;
    instance_ci.ppEnabledExtensionNames = instance_ext;
    instance_ci.pApplicationInfo = &app_info;

    /* Create a Vulkan instance */
    if (const VkResult r = vkCreateInstance(&instance_ci, nullptr, &instance); r != VK_SUCCESS) {
        if (r == VK_ERROR_LAYER_NOT_PRESENT) return Err("failed to create vulkan instance. (layer not supported)");
        if (r == VK_ERROR_EXTENSION_NOT_PRESENT) return Err("failed to create vulkan instance. (extension not supported)");
        return Err("failed to create vulkan instance.");
    }

    /* Load Vulkan instance API functions */
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

    /* Select the queue families we want from the physical device */
    if (const Result r = select_queue_families(physical_device); r.is_ok()) {
        queue_families = r.unwrap();
    } else return Err(r.unwrap_err());
    
    /* Log the selected queue families */
    this->log(DebugSeverity::Info, strfmt("selected queues: G%u C%u T%u", queue_families.queue_combined, queue_families.queue_compute, queue_families.queue_transfer).data());
    
    /* Vulkan device queue creation info */
    const float priority = 0.0f;
    VkDeviceQueueCreateInfo device_queues_ci[3] {};
    device_queues_ci[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queues_ci[0].queueFamilyIndex = queue_families.queue_combined;
    device_queues_ci[0].queueCount = 1u;
    device_queues_ci[0].pQueuePriorities = &priority;
    device_queues_ci[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queues_ci[1].queueFamilyIndex = queue_families.queue_compute;
    device_queues_ci[1].queueCount = 1u;
    device_queues_ci[1].pQueuePriorities = &priority;
    device_queues_ci[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queues_ci[2].queueFamilyIndex = queue_families.queue_transfer;
    device_queues_ci[2].queueCount = 1u;
    device_queues_ci[2].pQueuePriorities = &priority;

    /* Enable synchronization 2.0 features */
    VkPhysicalDeviceSynchronization2Features sync_features { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES };
    sync_features.synchronization2 = true;

    /* Enable dynamic rendering features */
    VkPhysicalDeviceDynamicRenderingFeatures render_features { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
    render_features.pNext = (void*)&sync_features;
    render_features.dynamicRendering = true;

    /* Vulkan device creation info */
    VkDeviceCreateInfo device_ci { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    device_ci.pNext = &render_features;
    device_ci.queueCreateInfoCount = 3u;
    device_ci.pQueueCreateInfos = device_queues_ci;
    device_ci.enabledLayerCount = instance_layers_count;
    device_ci.ppEnabledLayerNames = instance_layers;
    device_ci.enabledExtensionCount = device_ext_count;
    device_ci.ppEnabledExtensionNames = device_ext;

    /* Create a Vulkan logical device */
    if (const VkResult r = vkCreateDevice(physical_device, &device_ci, nullptr, &logical_device); r != VK_SUCCESS) {
        if (r == VK_ERROR_FEATURE_NOT_PRESENT) return Err("failed to create vulkan device. (feature not supported)");
        if (r == VK_ERROR_EXTENSION_NOT_PRESENT) return Err("failed to create vulkan device. (extension not supported)");
        return Err("failed to create vulkan device.");
    }

    /* Load Vulkan device API functions */
    volkLoadDevice(logical_device);

    /* Get the logical device queues */
    queues = get_queues(logical_device, queue_families);

    /* Command pool creation info */
    VkCommandPoolCreateInfo pool_ci{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_ci.queueFamilyIndex = queue_families.queue_combined;

    /* Create command pool for command buffers */
    if (vkCreateCommandPool(logical_device, &pool_ci, nullptr, &cmd_pool) != VK_SUCCESS) {
        return Err("failed to create command pool for render graph.");
    }

    /* Initialize VRAM banks */
    if (const Result r = init_vram_bank(); r.is_err()) {
        return Err(r.unwrap_err());
    }

    return Ok();
}

/* Destroy the GPU adapter, free all its resources. */
Result<void> GPUAdapter::destroy() {
    vkDestroyCommandPool(logical_device, cmd_pool, nullptr);

    vkDestroyDevice(logical_device, nullptr);
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
