#include "memory.hpp"

namespace vkw
{
    uint32_t DeviceMemory::findMemoryTypeIndex(const Device& device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        const VkPhysicalDeviceMemoryProperties memProperties = device.memoryProperties();
        for (int i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if (typeFilter & (1 << i) && properties ==
                (memProperties.memoryTypes[i].propertyFlags & properties))
            {
                return i;
            }
        }
        return UINT32_MAX;
    }

    DeviceMemory::DeviceMemory(const Device* device, VkDeviceSize size, uint32_t memoryType) :
        _device(device),
        _size(size)
    {
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = _size;
        allocInfo.memoryTypeIndex = memoryType;

        vkAllocateMemory(_device->device(), &allocInfo, NULL_ALLOC, &_memory);
    }

    DeviceMemory::~DeviceMemory()
    {
        if (_device) vkFreeMemory(_device->device(), _memory, NULL_ALLOC);
    }

    void DeviceMemory::writeToMemory(const void* data, VkDeviceSize size)
    {
        void* mappedData;
        vkMapMemory(_device->device(), _memory, 0, size, 0, &mappedData);
        memcpy(mappedData, data, size);
        vkUnmapMemory(_device->device(), _memory);
    }
}