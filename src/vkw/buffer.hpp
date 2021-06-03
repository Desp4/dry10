#pragma once

#ifndef DRY_VK_BUFFER_H
#define DRY_VK_BUFFER_H

#include "memory.hpp"

namespace dry::vkw {

class vk_buffer {
public:
    vk_buffer(const vk_device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

    vk_buffer() = default;
    vk_buffer(vk_buffer&& oth) { *this = std::move(oth); }
    ~vk_buffer();

    template<typename T>
    void write(const T& value) { _memory.write(&value, sizeof(T)); }
    template<typename T>
    void write(const std::vector<T>& values) { _memory.write(values.data(), sizeof(T) * values.size()); }
    template<typename T>
    void write(std::span<const T> values) { _memory.write(values.data(), values.size_bytes()); }
    void write(const void* data, VkDeviceSize size) { _memory.write(data, size); }

    VkBuffer handle() const { return _buffer; }
    VkDeviceMemory memory_handle() const { return _memory.handle(); }
    VkDeviceSize size() const { return _true_size; }

    vk_buffer& operator=(vk_buffer&&);

private:
    const vk_device* _device = nullptr;
    VkBuffer _buffer = VK_NULL_HANDLE;
    vk_device_memory _memory;
    VkDeviceSize _true_size = 0;
};

}

#endif
