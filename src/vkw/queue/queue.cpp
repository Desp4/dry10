#include "queue.hpp"

namespace dry::vkw {

vk_queue::vk_queue(const vk_device& device, u32_t queue_family_index, u32_t queue_index, VkCommandPoolCreateFlags flags) noexcept :
    _pool{ device, queue_family_index, flags }
{
    vkGetDeviceQueue(device.handle(), queue_family_index, queue_index, &_queue);
}

vk_cmd_buffer vk_queue::create_buffer() const {
    return _pool.create_buffer();
}

void vk_queue::submit(const vk_cmd_buffer& cmd) const {
    const auto cmd_h = cmd.handle();
    vkEndCommandBuffer(cmd_h);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_h;

    vkQueueSubmit(_queue, 1, &submit_info, VK_NULL_HANDLE);
}

void vk_queue::collect() const {
    vkQueueWaitIdle(_queue);
}

}