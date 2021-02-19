#pragma once

#include "vkw/cmd/cmdbuffer.hpp"

namespace dry::vkw {

// TODO : if doing multiple pools, ring buffers etc need to reflect that or just dump all queue functionality in free functions
class queue_base : public movable<queue_base> {
public:
    using movable<queue_base>::operator=;

    queue_base() = default;
    queue_base(queue_base&&) = default;
    queue_base(uint32_t queue_family_index, uint32_t queue_index);

    void submit_cmd(VkCommandBuffer cmd_buf) const;

    // NOTE : need these in renderer for allocating buffers and for swapchain only
    VkQueue queue() const {
        return _queue;
    }
    const cmd_pool& pool() const {
        return _pool;
    }

protected:
    VkQueue _queue;
    cmd_pool _pool;
};

}