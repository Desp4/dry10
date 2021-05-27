#include "buffer.hpp"

namespace dry::vkw {

vk_buffer::vk_buffer(const vk_device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) :
    _device{ &device },
    _true_size{ size }
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(_device->handle(), &buffer_info, null_alloc, &_buffer);

    // Get memory requirements
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(_device->handle(), _buffer, &mem_requirements);

    _memory = vk_device_memory{ *_device,mem_requirements.size, _device->find_memory_type_index(mem_requirements.memoryTypeBits, properties) };
    vkBindBufferMemory(_device->handle(), _buffer, _memory.handle(), 0);
}

vk_buffer::~vk_buffer() {
    if (_device != nullptr) {
        vkDestroyBuffer(_device->handle(), _buffer, null_alloc);
    }
}

vk_buffer& vk_buffer::operator=(vk_buffer&& oth) {
    // destroy
    if (_device != nullptr) {
        vkDestroyBuffer(_device->handle(), _buffer, null_alloc);
    }
    // move
    _device = oth._device;
    _buffer = oth._buffer;
    _memory = std::move(oth._memory);
    _true_size = oth._true_size;
    // null
    oth._device = nullptr;
    return *this;
}

}
