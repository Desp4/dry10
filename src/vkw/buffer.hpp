#pragma once

#ifndef DRY_VK_BUFFER_H
#define DRY_VK_BUFFER_H

#include "device/device.hpp"

namespace dry::vkw {

class vk_buffer {
public:
    vk_buffer(const vk_device& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) noexcept;

    vk_buffer() noexcept = default;
    vk_buffer(vk_buffer&& oth) noexcept { *this = std::move(oth); }
    ~vk_buffer();

    template<typename T>
    void write(std::span<const T> src);

    template<typename T = void>
    T* map() const;
    void unmap() const;

    VkBuffer handle() const { return _buffer; }
    VkDeviceSize size() const { return _true_size; }

    vk_buffer& operator=(vk_buffer&&) noexcept;

private:
    const vk_device* _device = nullptr;
    VkBuffer _buffer = VK_NULL_HANDLE;
    VmaAllocation _alloc = VK_NULL_HANDLE;
    VkDeviceSize _true_size = 0;
};



template<typename T>
void vk_buffer::write(std::span<const T> src) {
    auto dst = map<T>();
    std::copy(src.begin(), src.end(), dst);
    vmaUnmapMemory(_device->allocator(), _alloc);
}

template<typename T>
T* vk_buffer::map() const {
    T* dst = nullptr;
    vmaMapMemory(_device->allocator(), _alloc, reinterpret_cast<void**>(&dst));
    return dst;
}

}

#endif
