#pragma once

#include "device/device.hpp"

namespace vkw
{
    class DeviceMemory : public Movable<DeviceMemory>
    {
    public:
        // Returns UINT32_MAX on failure
        static uint32_t findMemoryTypeIndex(const Device& device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

        using Movable<DeviceMemory>::operator=;

        DeviceMemory() = default;
        DeviceMemory(DeviceMemory&&) = default;
        DeviceMemory(const Device* device, VkDeviceSize size, uint32_t memoryType);
        ~DeviceMemory();

        void writeToMemory(const void* data, VkDeviceSize size);

        VkDeviceMemory memory() const;
        VkDeviceSize size() const;

    private:
        DevicePtr _device;

        VkDeviceSize _size;
        VkHandle<VkDeviceMemory> _memory;
    };
}