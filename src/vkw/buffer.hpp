#pragma once

#include "memory.hpp"

namespace dry::vkw {

class buffer_base : public movable<buffer_base> {
public:
    using movable<buffer_base>::operator=;

    buffer_base() = default;
    buffer_base(buffer_base&&) = default;
    buffer_base(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~buffer_base();

    // TODO : if local buffer has the same functionality as a non-local one keep it, if not separate the two into different structures
    void write(const void* data, VkDeviceSize size);

    const VkBuffer& buffer() const {
        return _buffer;
    }
    const VkDeviceSize& size() const {
        return _true_size;
    }

private:
    vk_handle<VkBuffer> _buffer;
    device_memory _memory;
    VkDeviceSize _true_size;
};

}