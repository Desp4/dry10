#include "cmdpool.hpp"

namespace vkw
{
    CmdPool::CmdPool(const Device* device, uint32_t queue) :
        _device(device)
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queue;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // note

        vkCreateCommandPool(_device.ptr->device(), &poolInfo, NULL_ALLOC, &_pool.handle);
    }

    CmdPool::~CmdPool()
    {
        if (_device) vkDestroyCommandPool(_device.ptr->device(), _pool, NULL_ALLOC);
    }

    VkCommandPool CmdPool::pool() const
    {
        return _pool;
    }
}