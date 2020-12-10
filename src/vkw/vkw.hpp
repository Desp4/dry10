#pragma once

#ifdef _WIN32
  #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>
#include "../util.hpp"

#define NULL_ALLOC nullptr

namespace vkw
{
    using util::Movable;
    using util::NullablePtr;

    template<typename T>
    struct VkHandle
    {
        T handle;

        VkHandle() : handle(VK_NULL_HANDLE)
        {
        }

        VkHandle(const T& oth) : handle(oth)
        {
        }

        VkHandle(VkHandle&& oth) : handle(oth.handle)
        {
            oth.handle = VK_NULL_HANDLE;
        }

        operator T() const
        {
            return handle;
        }

        VkHandle& operator=(VkHandle&& oth)
        {
            handle = oth.handle;
            oth.handle = VK_NULL_HANDLE;
            return *this;
        }

        VkHandle& operator=(const T& oth)
        {
            handle = oth;
            return *this;
        }

        VkHandle(const VkHandle&) = delete;
        VkHandle& operator=(const VkHandle&) = delete;
    };
}