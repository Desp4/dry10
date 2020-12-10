#include "instance.hpp"

namespace vkw
{
    Instance::Instance(std::span<const char* const> extensions, std::span<const char* const> layers, DebugCallback callback, const char* exeName)
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_2;
        appInfo.pApplicationName = exeName;
        appInfo.pEngineName = "dry1";

        VkInstanceCreateInfo instanceInfo{};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo = &appInfo;
        instanceInfo.enabledExtensionCount = extensions.size();
        instanceInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
        const bool isDebug = layers.size() > 0 && callback;
        if (isDebug)
        {
            debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugInfo.pfnUserCallback = callback;

            instanceInfo.pNext = &debugInfo;
            instanceInfo.enabledLayerCount = layers.size();
            instanceInfo.ppEnabledLayerNames = layers.data();
        }

        vkCreateInstance(&instanceInfo, NULL_ALLOC, &_instance.handle);

        if (isDebug)
        {
            const auto createDebugger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT"));
            createDebugger(_instance, &debugInfo, NULL_ALLOC, &_debugger.handle);
        }
    }

    Instance::~Instance()
    {
        if (_instance)
        {
            const auto destroyDebugger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT"));
            destroyDebugger(_instance, _debugger, NULL_ALLOC);
        }
        vkDestroyInstance(_instance, NULL_ALLOC);
    }

    std::vector<VkPhysicalDevice> Instance::enumeratePhysicalDevices() const
    {
        uint32_t devCount;
        vkEnumeratePhysicalDevices(_instance, &devCount, nullptr);

        std::vector<VkPhysicalDevice> devices(devCount);
        vkEnumeratePhysicalDevices(_instance, &devCount, devices.data());
        return devices;
    }


    VkInstance Instance::instance() const
    {
        return _instance;
    }
}