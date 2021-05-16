#include "buffer.hpp"

#include "device/g_device.hpp"

namespace dry::vkw {

vk_buffer::vk_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) :
    _true_size{ size }
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(g_device->handle(), &buffer_info, null_alloc, &_buffer);

    // Get memory requirements
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(g_device->handle(), _buffer, &mem_requirements);

    _memory = vk_device_memory(mem_requirements.size, g_device->find_memory_type_index(mem_requirements.memoryTypeBits, properties));
    vkBindBufferMemory(g_device->handle(), _buffer, _memory.handle(), 0);
}

vk_buffer::~vk_buffer() {
    vkDestroyBuffer(g_device->handle(), _buffer, null_alloc);
}

vk_buffer& vk_buffer::operator=(vk_buffer&& oth) {
    // destroy
    vkDestroyBuffer(g_device->handle(), _buffer, null_alloc);
    // move
    _buffer = oth._buffer;
    _memory = std::move(oth._memory);
    _true_size = oth._true_size;
    // null
    oth._buffer = VK_NULL_HANDLE;
    return *this;
}

}
