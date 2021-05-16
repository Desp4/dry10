#include "memory.hpp"

#include "device/g_device.hpp"

namespace dry::vkw {

vk_device_memory::vk_device_memory(VkDeviceSize size, u32_t memory_type) :
    _size{ size }
{
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = _size;
    alloc_info.memoryTypeIndex = memory_type;

    vkAllocateMemory(g_device->handle(), &alloc_info, null_alloc, &_memory);
}

vk_device_memory::~vk_device_memory() {
    vkFreeMemory(g_device->handle(), _memory, null_alloc);
}

void vk_device_memory::write(const void* data, VkDeviceSize size) {
    void* mapped_data = nullptr;
    vkMapMemory(g_device->handle(), _memory, 0, size, 0, &mapped_data); // TODO : no offsets no nothing, rigid usage
    memcpy(mapped_data, data, size);
    vkUnmapMemory(g_device->handle(), _memory); // TODO : mapping and unmapping is slow, on UBOs etc leave mapped
}

vk_device_memory& vk_device_memory::operator=(vk_device_memory&& oth) {
    // destroy
    vkFreeMemory(g_device->handle(), _memory, null_alloc);
    // move
    _memory = oth._memory;
    _size = oth._size;
    // null
    oth._memory = VK_NULL_HANDLE;
    return *this;
}

}
