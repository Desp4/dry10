#include "cmdpool.hpp"

namespace dry::vkw {

vk_cmd_pool::vk_cmd_pool(const vk_device& device, u32_t queue) :
    _device{ &device }
{
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = queue;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // NOTE : this for rewriting used buffers

    vkCreateCommandPool(_device->handle(), &pool_info, null_alloc, &_pool);
}

vk_cmd_pool::~vk_cmd_pool() {
    if (_device != nullptr) {
        vkDestroyCommandPool(_device->handle(), _pool, null_alloc);
    }
}

vk_cmd_pool& vk_cmd_pool::operator=(vk_cmd_pool&& oth) {
    // destroy
    if (_device != nullptr) {
        vkDestroyCommandPool(_device->handle(), _pool, null_alloc);
    }
    // move
    _device = oth._device;
    _pool = oth._pool;
    // null
    oth._device = nullptr;
    return *this;
}

}