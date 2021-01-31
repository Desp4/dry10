#pragma once

#include "memory.hpp"

namespace vkw
{
    class Buffer : public Movable<Buffer>
    {
    public:
        using Movable<Buffer>::operator=;

        Buffer() = default;
        Buffer(Buffer&&) = default;
        Buffer(const Device* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        ~Buffer();

        // TODO : if local buffer has the same functionality as a non-local one keep it, if not separate the two into different structures
        void writeToMemory(const void* data, VkDeviceSize size);

        const VkHandle<VkBuffer>& buffer() const { return _buffer; }
        const VkDeviceSize& size() const { return _trueSize; }

    private:
        DevicePtr _device;

        VkHandle<VkBuffer> _buffer;
        DeviceMemory _memory;
        VkDeviceSize _trueSize;
    };
}