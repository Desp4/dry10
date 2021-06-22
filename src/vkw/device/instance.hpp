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

    static constexpr u32_t api_version = VK_API_VERSION_1_2;

    // name can be null
    vk_instance(std::span<const char* const> extensions, const char* name,
        std::span<const char* const> layers, debug_callback callback
    ) noexcept;
    vk_instance(std::span<const char* const> extensions, const char* name) noexcept :
        vk_instance{ extensions, name, std::span<const char* const>{}, nullptr } {}

    vk_instance() noexcept = default;
    vk_instance(vk_instance&& oth) noexcept { *this = std::move(oth); }
    ~vk_instance();

    std::vector<VkPhysicalDevice> enumerate_physical_devices() const;

    VkInstance handle() const { return _instance; }

    vk_instance& operator=(vk_instance&&) noexcept;

private:
    VkInstance _instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugger = VK_NULL_HANDLE;
};

}

#endif
