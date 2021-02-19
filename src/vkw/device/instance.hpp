#pragma once

#include <vector>
#include <span>

#include "vkw/vkw.hpp"

namespace dry::vkw {

class instance_main {
public:
    using debug_callback = VKAPI_ATTR VkBool32(VKAPI_CALL*)(
        VkDebugUtilsMessageSeverityFlagBitsEXT,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT*,
        void*
    );

    static void create(std::span<const char* const> extensions, std::span<const char* const> layers,
                       debug_callback callback, const char* name = nullptr);
    static void destroy();

    static std::vector<VkPhysicalDevice> enumerate_physical_devices();

    static const VkInstance& instance() {
        return _instance;
    }

private:
    instance_main() = default;

    inline static VkInstance _instance = VK_NULL_HANDLE;
    inline static VkDebugUtilsMessengerEXT _debugger = VK_NULL_HANDLE;

    inline static bool _destroyed = false;
};

}