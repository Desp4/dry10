#include "queue.hpp"

namespace vkw
{
    Queue::Queue(const Device* device, uint32_t queueFamilyIndex, uint32_t queueIndex) :
        _pool(device, queueFamilyIndex)
    {
        vkGetDeviceQueue(device->device(), queueFamilyIndex, queueIndex, &_queue);
    }

    void Queue::submitCmd(VkCommandBuffer cmdBuffer) const
    {
        vkEndCommandBuffer(cmdBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(_queue); // NOTE : waiting here, perhaps not needed in some cases
    }
}