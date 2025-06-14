#include "extensions_vk.hh"

#include <cstring> /* strcmp */

/* Query whether the driver supports debug utils. (needed for the debug messenger) */
bool query_debug_support(const char* layer_name) {
    u32 n = 0; /* Query all supported extensions */
    vkEnumerateInstanceExtensionProperties(layer_name, &n, nullptr);
    VkExtensionProperties* extensions = new VkExtensionProperties[n] {};
    vkEnumerateInstanceExtensionProperties(layer_name, &n, extensions);

    /* Check if the driver supports the debug utils extension (for validation layer logging) */
    bool supported = false;
    for (u32 i = 0u; i < n; ++i) {
        if (strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extensions[i].extensionName) == 0) { 
            supported = true;
            break;
        }
    }

    delete[] extensions; /* Free the extensions list */
    return supported;
}

/* Query whether the driver supports our required instance extensions. */
Result<void> query_instance_support(const char* const* extensions, const u32 count) {
    u32 n = 0; /* Query all supported extensions */
    vkEnumerateInstanceExtensionProperties(nullptr, &n, nullptr);
    VkExtensionProperties* supported_extensions = new VkExtensionProperties[n] {};
    vkEnumerateInstanceExtensionProperties(nullptr, &n, supported_extensions);

    /* Check if the driver supports the instance extensions */
    for (u32 i = 0u; i < count; ++i) {
        bool found = false;
        for (u32 j = 0u; j < n; ++j) {
            if (strcmp(extensions[i], supported_extensions[j].extensionName) == 0) { 
                found = true;
                break;
            }
        }
        if (found == false) {
            delete[] supported_extensions; /* Free the extensions list */
            return Err("instance does not support the '%s' extension.", extensions[i]);
        }
    }

    delete[] supported_extensions; /* Free the extensions list */
    return Ok();
}

/* Query whether the instance supports validation layers. */
bool query_validation_support(const char* layer_name) {
    u32 n = 0; /* Query all supported extensions */
    vkEnumerateInstanceLayerProperties(&n, nullptr);
    VkLayerProperties* layers = new VkLayerProperties[n] {};
    vkEnumerateInstanceLayerProperties(&n, layers);

    /* Check if the driver supports the khronos validation layer */
    bool supported = false;
    for (u32 i = 0u; i < n; ++i) {
        if (strcmp(layer_name, layers[i].layerName) == 0) { 
            supported = true;
            break;
        }
    }

    delete[] layers; /* Free the layers list */
    return supported;
}

Result<void> query_extension_support(const VkPhysicalDevice device, const char* const* extensions, const u32 count) {
    u32 n = 0; /* Query all supported extensions */
    vkEnumerateDeviceExtensionProperties(device, nullptr, &n, nullptr);
    VkExtensionProperties* supported_extensions = new VkExtensionProperties[n] {};
    vkEnumerateDeviceExtensionProperties(device, nullptr, &n, supported_extensions);

    /* Check if the driver supports the debug utils extension (for validation layer logging) */
    for (u32 i = 0u; i < count; ++i) {
        bool found = false;
        for (u32 j = 0u; j < n; ++j) {
            if (strcmp(extensions[i], supported_extensions[j].extensionName) == 0) { 
                found = true;
                break;
            }
        }
        if (found == false) {
            delete[] supported_extensions; /* Free the extensions list */
            return Err("device does not support the '%s' extension.", extensions[i]);
        }
    }

    delete[] supported_extensions; /* Free the extensions list */
    return Ok();
}
