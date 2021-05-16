#pragma once

#ifndef DRY_VK_CMDBUFFER_H
#define DRY_VK_CMDBUFFER_H

#include "cmdpool.hpp"

namespace dry::vkw {

class vk_cmd_buffer {
public:
    vk_cmd_buffer(const vk_cmd_pool& pool);

    vk_cmd_buffer() = default;
    vk_cmd_buffer(vk_cmd_buffer&& oth) { *this = std::move(oth); }
    ~vk_cmd_buffer();

    // defaults to none
    void begin(VkCommandBufferUsageFlags usage = 0) const;

    VkCommandBuffer handle() const { return _buffer; }

    vk_cmd_buffer& operator=(vk_cmd_buffer&&);

private:
    const vk_cmd_pool* _pool = nullptr;
    VkCommandBuffer _buffer = VK_NULL_HANDLE;
};

}

#endif
