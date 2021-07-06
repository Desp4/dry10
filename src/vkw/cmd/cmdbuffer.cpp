#include "cmdbuffer.hpp"

#include "cmdpool.hpp"

namespace dry::vkw {

vk_cmd_buffer::vk_cmd_buffer(const vk_device& device, const vk_cmd_pool& pool) :
    _device{ &device },
    _pool{ &pool }
{
    VkCommandBufferAllocateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_info.commandPool = _pool->handle();
    buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_info.commandBufferCount = 1; // TODO : allocating 1, CAN do in batches pls
    vkAllocateCommandBuffers(_device->handle(), &buffer_info, &_buffer);
}

vk_cmd_buffer::~vk_cmd_buffer() {
    if (_device != nullptr) {
        vkFreeCommandBuffers(_device->handle(), _pool->handle(), 1, &_buffer); // NOTE : 1 buffer, hardcoded
    }   
}

void vk_cmd_buffer::begin(VkCommandBufferUsageFlags usage) const {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = usage;
    vkBeginCommandBuffer(_buffer, &begin_info);
}

vk_cmd_buffer& vk_cmd_buffer::operator=(vk_cmd_buffer&& oth) {
    // destroy
    if (_device != nullptr) {
        vkFreeCommandBuffers(_device->handle(), _pool->handle(), 1, &_buffer);
    }
    // move
    _device = oth._device;
    _pool = oth._pool;
    _buffer = oth._buffer;
    // null
    oth._device = nullptr;
    return *this;
}

}
