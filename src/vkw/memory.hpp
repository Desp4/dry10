#pragma once

#ifndef DRY_VK_MEMORY_H
#define DRY_VK_MEMORY_H

#include "vkw/device/device.hpp"

namespace dry::vkw {

class vk_device_memory {
public:
    vk_device_memory(const vk_device& device, VkDeviceSize size, u32_t memory_type);

    vk_device_memory() = default;
    vk_device_memory(vk_device_memory&& oth) { *this = std::move(oth); }
    ~vk_device_memory();

    template<typename T>
    void write(const T& value) { write(&value, sizeof(T)); }
    template<typename T>
    void write(const std::vector<T>& values) { write(values.data(), sizeof(T) * values.size()); }
    template<typename T>
    void write(std::span<const T> values) { write(values.data(), values.size_bytes()); }
    void write(const void* data, VkDeviceSize size);

    VkDeviceMemory handle() const { return _memory; }
    VkDeviceSize size() const { return _size; }

    vk_device_memory& operator=(vk_device_memory&&);

private:
    const vk_device* _device = nullptr;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkDeviceSize _size = 0;
};

}

#endif
