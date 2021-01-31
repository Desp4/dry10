#pragma once

#include "device/device.hpp"

namespace vkw
{
    class DeviceMemory : public Movable<DeviceMemory>
    {
    public:
        using Movable<DeviceMemory>::operator=;

        // returns UINT32_MAX on failure
        static uint32_t findMemoryTypeIndex(const Device& device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

        DeviceMemory() = default;
        DeviceMemory(DeviceMemory&&) = default;
        DeviceMemory(const Device* device, VkDeviceSize size, uint32_t memoryType);
        ~DeviceMemory();

        void writeToMemory(const void* data, VkDeviceSize size);

        const VkHandle<VkDeviceMemory>& memory() const { return _memory; }
        const VkDeviceSize& size() const { return _size; }

    private:
        DevicePtr _device;

        VkDeviceSize _size;
        VkHandle<VkDeviceMemory> _memory;
    };
}