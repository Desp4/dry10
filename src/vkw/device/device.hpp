#pragma once

#include <span>

#include "../vkw.hpp"

namespace vkw
{
    class Device : public Movable<Device>
    {
    public:
        using Movable<Device>::operator=;

        Device() = default;
        Device(Device&&) = default;
        Device(VkPhysicalDevice physDevice, std::span<const uint32_t> queues,
               std::span<const char* const> extensions, const VkPhysicalDeviceFeatures& features);
        ~Device();

        VkDevice device() const;
        VkPhysicalDeviceMemoryProperties memoryProperties() const;

    private:
        VkHandle<VkDevice> _device;
        VkHandle<VkPhysicalDevice> _physDevice;
    };

    using DevicePtr = NullablePtr<const Device>;
}