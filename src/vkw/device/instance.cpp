#include "instance.hpp"
#include "dbg/log.hpp"

namespace dry::vkw {

instance::instance(std::span<const char* const> extensions, std::span<const char* const> layers, debug_callback callback, const char* name) {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion = VK_API_VERSION_1_2;
    app_info.pApplicationName = name;
    app_info.pEngineName = "dry1";

    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instance_info.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_info{};
    const bool is_debug = layers.size() > 0 && callback;
    if (is_debug) {
        debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_info.pfnUserCallback = callback;

        instance_info.pNext = &debug_info;
        instance_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
        instance_info.ppEnabledLayerNames = layers.data();
    }

    vkCreateInstance(&instance_info, NULL_ALLOC, &_instance);
    LOG_DBG(
        "vulkan instance for dry1 created\n\tapplication name: %s\n\tapi version: %i.%i.%i\n\tvalidation layers: %s",
        name,
        app_info.apiVersion >> 22, (app_info.apiVersion >> 12) & 0xFFF, app_info.apiVersion & 0xFFF,
        is_debug ? "enabled" : "disabled"
    );

    if (is_debug) {
        const auto create_debugger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT"));
        create_debugger(_instance, &debug_info, NULL_ALLOC, &_debugger);
    }
}

instance::~instance() {
    const auto destroy_debugger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT"));
    destroy_debugger(_instance, _debugger, NULL_ALLOC);
    vkDestroyInstance(_instance, NULL_ALLOC);
}

std::vector<VkPhysicalDevice> instance::enumerate_physical_devices() const {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(_instance, &device_count, nullptr);

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(_instance, &device_count, devices.data());
    return devices;
}

}