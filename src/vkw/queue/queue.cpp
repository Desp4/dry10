#include "queue.hpp"
#include "vkw/device/device.hpp"

namespace dry::vkw {

queue_base::queue_base(uint32_t queue_family_index, uint32_t queue_index) :
    _pool(queue_family_index)
{
    vkGetDeviceQueue(device_main::device(), queue_family_index, queue_index, &_queue);
}

void queue_base::submit_cmd(VkCommandBuffer cmd_buf) const {
    vkEndCommandBuffer(cmd_buf);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buf;

    vkQueueSubmit(_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(_queue); // NOTE : waiting here, perhaps not needed in some cases
}

}