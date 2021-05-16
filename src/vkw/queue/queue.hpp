#pragma once

#ifndef DRY_VK_QUEUE_H
#define DRY_VK_QUEUE_H

#include "vkw/cmd/cmdbuffer.hpp"

namespace dry::vkw {

// TODO : if doing multiple pools, ring buffers etc need to reflect that or just dump all queue functionality in free functions
class vk_queue {
public:
    vk_queue(u32_t queue_family_index, u32_t queue_index);

    vk_queue() = default;
    vk_queue(vk_queue&& oth) { *this = std::move(oth); }

    void submit_cmd(VkCommandBuffer cmd_buf) const;

    // NOTE : need these in renderer for allocating buffers and for swapchain only
    VkQueue handle() const { return _queue; }
    const vk_cmd_pool& cmd_pool() const { return _pool; }

    vk_queue& operator=(vk_queue&&);

protected:
    VkQueue _queue = VK_NULL_HANDLE;
    vk_cmd_pool _pool;
};

}

#endif
