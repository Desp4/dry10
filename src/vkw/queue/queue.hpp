#pragma once

#ifndef DRY_VK_QUEUE_H
#define DRY_VK_QUEUE_H

#include "vkw/cmd/cmdpool.hpp"

namespace dry::vkw {

// TODO : if doing multiple pools, ring buffers etc need to reflect that
class vk_queue {
public:
    vk_queue(const vk_device& device, u32_t queue_family_index, u32_t queue_index, VkCommandPoolCreateFlags flags = 0) noexcept;

    vk_queue() noexcept = default;

    vk_cmd_buffer create_buffer() const;
    // submit one buffer, no fences no nothing, purely convenience for trivial submissions
    void submit(const vk_cmd_buffer& cmd) const;
    void collect() const;

    // NOTE : need these in renderer for allocating buffers and for swapchain only
    VkQueue handle() const { return _queue; }

private:
    VkQueue _queue = VK_NULL_HANDLE;
    vk_cmd_pool _pool;
};

}

#endif
