#include "buffer.hpp"

namespace vkw
{
    Buffer::Buffer(const Device* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) :
        _device(device),
        _trueSize(size)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateBuffer(_device->device(), &bufferInfo, NULL_ALLOC, &_buffer);

        // Get memory requirements
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(_device->device(), _buffer, &memRequirements);

        _memory = DeviceMemory(device, memRequirements.size,
            DeviceMemory::findMemoryTypeIndex(*_device, memRequirements.memoryTypeBits, properties));

        vkBindBufferMemory(_device->device(), _buffer, _memory.memory(), 0);
    }

    Buffer::~Buffer()
    {
        if (_device) vkDestroyBuffer(_device->device(), _buffer, NULL_ALLOC);
    }

    void Buffer::writeToMemory(const void* data, VkDeviceSize size)
    {
        _memory.writeToMemory(data, size);
    }
}