#include "queue.hpp"

namespace dry::vkw {

vk_queue::vk_queue(const vk_device& device, u32_t queue_family_index, u32_t queue_index) noexcept :
    _device{ &device },
    _pool{ device, queue_family_index }
{
    vkGetDeviceQueue(_device->handle(), queue_family_index, queue_index, &_queue);
}

void vk_queue::submit_cmd(VkCommandBuffer cmd_buf, bool wait) const {
    vkEndCommandBuffer(cmd_buf);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buf;

    vkQueueSubmit(_queue, 1, &submit_info, VK_NULL_HANDLE);
    if (wait) {
        wait_on_queue();
    }
}

vk_queue& vk_queue::operator=(vk_queue&& oth) noexcept {
    // delete
    // don't need to
    // move
    _device = oth._device;
    _queue = oth._queue;
    _pool = std::move(oth._pool);
    // null
    oth._device = nullptr;
    oth._queue = VK_NULL_HANDLE; // doing it just for consistentcy, cleaner errors
    return *this;
}

}