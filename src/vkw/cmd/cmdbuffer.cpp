#include "cmdbuffer.hpp"
#include "vkw/device/device.hpp"

namespace dry::vkw {

cmd_buffer::cmd_buffer(const cmd_pool* pool) :
    _pool(pool)
{
    VkCommandBufferAllocateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_info.commandPool = _pool->pool();
    buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_info.commandBufferCount = 1;
    vkAllocateCommandBuffers(device_main::device(), &buffer_info, &_buffer);
}

cmd_buffer::~cmd_buffer() {
    if (_pool != nullptr) {
        vkFreeCommandBuffers(device_main::device(), _pool->pool(), 1, &_buffer);
    }   
}

void cmd_buffer::begin(VkCommandBufferUsageFlags usage) const {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = usage;
    vkBeginCommandBuffer(_buffer, &begin_info);
}

}