#pragma once

#ifndef DRY_VKW_H
#define DRY_VKW_H

#include <vulkan/vulkan.h>

#include "util/util.hpp"

namespace dry::vkw {

constexpr VkAllocationCallbacks* null_alloc = nullptr;

template<typename T>
struct vk_handle : nullable_primitive<T, static_cast<T>(VK_NULL_HANDLE)> {
    using nullable_primitive<T, static_cast<T>(VK_NULL_HANDLE)>::nullable_primitive;

    vk_handle(const vk_handle&) = delete;
    vk_handle(vk_handle&&) = default;

    vk_handle& operator=(const vk_handle&) = delete;
    vk_handle& operator=(vk_handle&&) = default;
};

}

#endif
