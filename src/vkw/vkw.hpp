#pragma once

#include <vulkan/vulkan.h>

#include "util/util.hpp"

#define NULL_ALLOC nullptr

namespace dry::vkw {

using util::movable;
using util::nullable_ptr;

template<typename T>
struct vk_handle : public util::nullable_base<T, static_cast<T>(VK_NULL_HANDLE)> {
    using util::nullable_base<T, static_cast<T>(VK_NULL_HANDLE)>::nullable_base;

    vk_handle(const vk_handle&) = delete;
    vk_handle(vk_handle&&) = default;

    vk_handle& operator=(const vk_handle&) = delete;
    vk_handle& operator=(vk_handle&&) = default;
};

}