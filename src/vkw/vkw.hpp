#pragma once

#include <vulkan/vulkan.h>

#include "util/util.hpp"

#define NULL_ALLOC nullptr

namespace vkw
{
    using util::Movable;
    using util::NullablePtr;

    template<typename T>
    struct VkHandle
    {
        T handle;

        VkHandle() : handle(VK_NULL_HANDLE) {}
        explicit VkHandle(T oth) : handle(oth) {}
        VkHandle(VkHandle&& oth) : handle(oth.handle)
        {
            oth.handle = VK_NULL_HANDLE;
        }

        operator T() const { return handle; }

        const T* operator&() const { return &handle; }
        T* operator&() { return &handle; }

        VkHandle& operator=(VkHandle&& oth)
        {
            handle = oth.handle;
            oth.handle = VK_NULL_HANDLE;
            return *this;
        }

        VkHandle& operator=(T oth)
        {
            handle = oth;
            return *this;
        }

        VkHandle(const VkHandle&) = delete;
        VkHandle& operator=(const VkHandle&) = delete;
    };
}