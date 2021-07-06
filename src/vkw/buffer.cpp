#include "buffer.hpp"

namespace dry::vkw {

vk_buffer::vk_buffer(const vk_device& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) noexcept :
    _device{ &device },
    _true_size{ size }
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = memory_usage;

    vmaCreateBuffer(device.allocator(), &buffer_info, &alloc_info, &_buffer, &_alloc, nullptr);
}

void vk_buffer::unmap() const {
    vmaUnmapMemory(_device->allocator(), _alloc);
}

vk_buffer::~vk_buffer() {
    if (_device != nullptr) {
        vmaDestroyBuffer(_device->allocator(), _buffer, _alloc);
    }
}

vk_buffer& vk_buffer::operator=(vk_buffer&& oth) noexcept {
    // destroy
    if (_device != nullptr) {
        vmaDestroyBuffer(_device->allocator(), _buffer, _alloc);
    }
    // move
    _device = oth._device;
    _buffer = oth._buffer;
    _alloc = oth._alloc;
    _true_size = oth._true_size;
    // null
    oth._device = nullptr;
    // TODO : need this for validity checks, consider adding to others
    // and a generic function that checks if given type instance is valid
    oth._buffer = VK_NULL_HANDLE;
    return *this;
}

}
