#pragma once

#ifndef DRY_VK_CMDPOOL_H
#define DRY_VK_CMDPOOL_H

#include "cmdbuffer.hpp"

namespace dry::vkw {

class vk_cmd_pool {
public:
    vk_cmd_pool(const vk_device& device, u32_t queue, VkCommandPoolCreateFlags flags = 0);

    vk_cmd_pool() = default;
    vk_cmd_pool(vk_cmd_pool&& oth) { *this = std::move(oth); }
    ~vk_cmd_pool();

    vk_cmd_buffer create_buffer() const;

    VkCommandPool handle() const { return _pool; }

    vk_cmd_pool& operator=(vk_cmd_pool&&);

private:
    const vk_device* _device = nullptr;
    VkCommandPool _pool = VK_NULL_HANDLE;
};

}

#endif
