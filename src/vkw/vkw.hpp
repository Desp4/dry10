#pragma once

#ifndef DRY_VKW_H
#define DRY_VKW_H

#ifdef _DEBUG
  #ifndef VKW_ENABLE_DEBUG
    #define VKW_ENABLE_DEBUG
  #endif
#endif

#include <vulkan/vulkan.h>

#include "util/util.hpp"

namespace dry::vkw {

constexpr VkAllocationCallbacks* null_alloc = nullptr;

}

#endif
