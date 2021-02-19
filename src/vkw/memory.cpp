#include "memory.hpp"
#include "device/device.hpp"

namespace dry::vkw {

device_memory::device_memory(VkDeviceSize size, uint32_t memory_type) :
    _size(size)
{
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = _size;
    alloc_info.memoryTypeIndex = memory_type;

    vkAllocateMemory(device_main::device(), &alloc_info, NULL_ALLOC, &_memory);
}

device_memory::~device_memory() {
    vkFreeMemory(device_main::device(), _memory, NULL_ALLOC);
}

void device_memory::write(const void* data, VkDeviceSize size) {
    void* mapped_data = nullptr;
    vkMapMemory(device_main::device(), _memory, 0, size, 0, &mapped_data);
    memcpy(mapped_data, data, size);
    vkUnmapMemory(device_main::device(), _memory);
}

}