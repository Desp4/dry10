#pragma once

#ifndef DRY_VK_INSTANCE_H
#define DRY_VK_INSTANCE_H

#include "vkw/vkw.hpp"

namespace dry::vkw {

class vk_instance {
public:
    using debug_callback = VKAPI_ATTR VkBool32(VKAPI_CALL*)(
        VkDebugUtilsMessageSeverityFlagBitsEXT,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT*,
        void*
    );

    // name can be null
    vk_instance(std::span<const char* const> extensions, const char* name,
        std::span<const char* const> layers, debug_callback callback
    );
    vk_instance(std::span<const char* const> extensions, const char* name) :
        vk_instance{ extensions, name, std::span<const char* const>{}, nullptr } {}

    vk_instance() = default;
    vk_instance(vk_instance&& oth) { *this = std::move(oth); }
    ~vk_instance();

    std::vector<VkPhysicalDevice> enumerate_physical_devices() const;

    VkInstance handle() const { return _instance; }

    vk_instance& operator=(vk_instance&&);

private:
    VkInstance _instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugger = VK_NULL_HANDLE;
};

}

#endif
