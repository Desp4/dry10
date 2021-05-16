#include "queue.hpp"

#include "vkw/device/g_device.hpp"

namespace dry::vkw {

vk_queue::vk_queue(u32_t queue_family_index, u32_t queue_index) :
    _pool{ queue_family_index }
{
    vkGetDeviceQueue(g_device->handle(), queue_family_index, queue_index, &_queue);
}

void vk_queue::submit_cmd(VkCommandBuffer cmd_buf) const {
    vkEndCommandBuffer(cmd_buf);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buf;

    vkQueueSubmit(_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(_queue); // NOTE : waiting here, perhaps not needed in some cases
}

vk_queue& vk_queue::operator=(vk_queue&& oth) {
    // delete
    // don't need to
    // move
    _queue = oth._queue;
    _pool = std::move(oth._pool);
    // null
    oth._queue = VK_NULL_HANDLE; // doing it just for consistentcy, cleaner errors
    return *this;
}

}