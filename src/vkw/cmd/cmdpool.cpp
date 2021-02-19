#include "cmdpool.hpp"
#include "vkw/device/device.hpp"

namespace dry::vkw {

cmd_pool::cmd_pool(uint32_t queue) {
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = queue;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // NOTE : this for rewriting used buffers

    vkCreateCommandPool(device_main::device(), &pool_info, NULL_ALLOC, &_pool);
}

cmd_pool::~cmd_pool() {
    vkDestroyCommandPool(device_main::device(), _pool, NULL_ALLOC);
}

}