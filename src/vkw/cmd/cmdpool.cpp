#include "cmdpool.hpp"

#include "vkw/device/g_device.hpp"

namespace dry::vkw {

vk_cmd_pool::vk_cmd_pool(u32_t queue) {
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = queue;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // NOTE : this for rewriting used buffers

    vkCreateCommandPool(g_device->handle(), &pool_info, null_alloc, &_pool);
}

vk_cmd_pool::~vk_cmd_pool() {
    vkDestroyCommandPool(g_device->handle(), _pool, null_alloc);
}

vk_cmd_pool& vk_cmd_pool::operator=(vk_cmd_pool&& oth) {
    // destroy
    vkDestroyCommandPool(g_device->handle(), _pool, null_alloc);
    // move
    _pool = oth._pool;
    // null
    oth._pool = VK_NULL_HANDLE;
    return *this;
}

}