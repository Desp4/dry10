#pragma once

#include <vector>
#include <span>

#include "vkw/vkw.hpp"

namespace dry::vkw {

class instance : public movable<instance> {
public:
    using debug_callback = VKAPI_ATTR VkBool32(VKAPI_CALL*)(
        VkDebugUtilsMessageSeverityFlagBitsEXT,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT*,
        void*
    );

    using movable<instance>::operator=;

    instance() = default;
    instance(instance&&) = default;
    instance(std::span<const char* const> extensions, std::span<const char* const> layers,
             debug_callback callback, const char* name = nullptr);
    ~instance();

    std::vector<VkPhysicalDevice> enumerate_physical_devices() const;

    const VkInstance& vk_instance() const {
        return _instance;
    }

private:
    vk_handle<VkInstance> _instance;
    VkDebugUtilsMessengerEXT _debugger = VK_NULL_HANDLE;
};

}