#pragma once

#ifndef DRY_VK_BUFFER_H
#define DRY_VK_BUFFER_H

#include "memory.hpp"

namespace dry::vkw {

class vk_buffer {
public:
    vk_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

    vk_buffer() = default;
    vk_buffer(vk_buffer&& oth) { *this = std::move(oth); }
    ~vk_buffer();

    // TODO : if local buffer has the same functionality as a non-local one keep it, if not separate the two into different structures
    template<typename T>
    void write(const T& value) { _memory.write(&value, sizeof(T)); }
    template<typename T>
    void write(const std::vector<T>& values) { _memory.write(values.data(), sizeof(T) * values.size()); }
    template<typename T>
    void write(std::span<const T> values) { _memory.write(values.data(), values.size_bytes()); }
    void write(const void* data, VkDeviceSize size) { _memory.write(data, size); }

    VkBuffer handle() const { return _buffer; }
    VkDeviceSize size() const { return _true_size; }

    vk_buffer& operator=(vk_buffer&&);

private:
    VkBuffer _buffer = VK_NULL_HANDLE;
    vk_device_memory _memory;
    VkDeviceSize _true_size = 0;
};

}

#endif
