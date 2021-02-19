#pragma once

#include "vkw/vkw.hpp"

namespace dry::vkw {

class cmd_pool : public movable<cmd_pool> {
public:
    using movable<cmd_pool>::operator=;

    cmd_pool() = default;
    cmd_pool(cmd_pool&&) = default;
    cmd_pool(uint32_t queue);
    ~cmd_pool();

    const VkCommandPool& pool() const {
        return _pool;
    }

private:
    vk_handle<VkCommandPool> _pool;
};

}