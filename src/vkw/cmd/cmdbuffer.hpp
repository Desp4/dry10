#pragma once

#include <vector>

#include "cmdpool.hpp"

namespace dry::vkw {

class cmd_buffer : public movable<cmd_buffer> {
public:
    using movable<cmd_buffer>::operator=;

    cmd_buffer() = default;
    cmd_buffer(cmd_buffer&&) = default;
    cmd_buffer(const cmd_pool* pool);
    ~cmd_buffer();

    void begin(VkCommandBufferUsageFlags usage) const;

    const VkCommandBuffer& buffer() const {
        return _buffer;
    }

private:
    nullable_ptr<const cmd_pool> _pool;
    vk_handle<VkCommandBuffer> _buffer;
};

}