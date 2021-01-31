#pragma once

#include <span>
#include <vector>

#include "vkw/vkw.hpp"

namespace vkw
{
    class Device : public Movable<Device>
    {
    public:
        struct QueueInfo
        {
            uint32_t queueFamilyIndex;
            uint32_t queueCount;
            std::vector<float> priorities;
        };

        using Movable<Device>::operator=;

        Device() = default;
        Device(Device&&) = default;
        Device(VkPhysicalDevice physDevice, std::span<const QueueInfo> queueInfos,
               std::span<const char* const> extensions, const VkPhysicalDeviceFeatures& features);
        ~Device();

        VkSurfaceCapabilitiesKHR surfaceCapabilities(VkSurfaceKHR surface) const;
        VkPhysicalDeviceMemoryProperties memoryProperties() const;

        const VkHandle<VkDevice>& device() const { return _device; }

    private:
        VkHandle<VkDevice> _device;
        VkHandle<VkPhysicalDevice> _physDevice;
    };

    using DevicePtr = NullablePtr<const Device>;
}