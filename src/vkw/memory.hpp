#pragma once

#include "vkw/vkw.hpp"

namespace dry::vkw {

class device_memory : public movable<device_memory> {
public:
    using movable<device_memory>::operator=;

    device_memory() = default;
    device_memory(device_memory&&) = default;
    device_memory(VkDeviceSize size, uint32_t memory_type);
    ~device_memory();

    void write(const void* data, VkDeviceSize size);

    const VkDeviceMemory& memory() const {
        return _memory;
    }
    const VkDeviceSize& size() const {
        return _size;
    }

private:
    VkDeviceSize _size;
    vk_handle<VkDeviceMemory> _memory;
};

}