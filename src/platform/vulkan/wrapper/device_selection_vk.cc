#include "device_selection_vk.hh"

#include "extensions_vk.hh"

/* Rank a physical device by its type. */
inline int rank_device_type(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties props {};
    vkGetPhysicalDeviceProperties(physical_device, &props);
    return device_ranking[props.deviceType];
}

/* Select the most suitable, available, physical device. */
Result<VkPhysicalDevice> select_physical_device(VkInstance instance, const char* const* extensions, const u32 count) {
    /* Get the number of available physical devices */
    u32 device_count = 0u;
    if (vkEnumeratePhysicalDevices(instance, &device_count, nullptr) != VK_SUCCESS) {
        return Err("failed to query available physical device count.");
    }

    /* Exit if no devices were found */
    if (device_count == 0u) return Err("no available physical devices.");

    /* Get a list of available physical devices */
    VkPhysicalDevice* devices = new VkPhysicalDevice[device_count];
    if (vkEnumeratePhysicalDevices(instance, &device_count, devices) != VK_SUCCESS) {
        delete[] devices;
        return Err("failed to query available physical devices.");
    }

    /* Select the most suitable physical device */
    VkPhysicalDevice selected = nullptr;
    for (u32 i = 0u; i < device_count; ++i) {
        /* Query if this physical device supports all required extensions */
        if (query_extension_support(devices[i], extensions, count).is_err()) continue;

        /* Get some info on the physical devices */
        const int type = selected == nullptr ? -1 : rank_device_type(selected);
        const int c_type = rank_device_type(devices[i]);

        /* If the candidate device ranks higher, it becomes the newly selected device */
        if (c_type > type) selected = devices[i];
    }

    delete[] devices;
    if (selected == nullptr) return Err("failed to find a suitable physical device.");
    return Ok(selected);
}

/* Get the name of a physical device. */
std::string get_physical_device_name(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties props {};
    vkGetPhysicalDeviceProperties(physical_device, &props);
    return std::string(props.deviceName);
}
