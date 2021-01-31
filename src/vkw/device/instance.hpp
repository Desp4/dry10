#pragma once

#include <vector>
#include <span>

#include "vkw/vkw.hpp"

namespace vkw
{
    class Instance : public Movable<Instance>
    {
    public:
        using DebugCallback = VKAPI_ATTR VkBool32(VKAPI_CALL*)(
            VkDebugUtilsMessageSeverityFlagBitsEXT,
            VkDebugUtilsMessageTypeFlagsEXT,
            const VkDebugUtilsMessengerCallbackDataEXT*,
            void*);

        using Movable<Instance>::operator=;
        
        Instance() = default;
        Instance(Instance&&) = default;
        Instance(std::span<const char* const> extensions, std::span<const char* const> layers, DebugCallback callback, const char* exeName = nullptr);
        ~Instance();

        std::vector<VkPhysicalDevice> enumeratePhysicalDevices() const;

        const VkHandle<VkInstance>& instance() const { return _instance; }

    private:
        VkHandle<VkInstance> _instance;
        VkHandle<VkDebugUtilsMessengerEXT> _debugger;
    };
}