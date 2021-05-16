#pragma once

#ifndef DRY_VK_SURFACE_H
#define DRY_VK_SURFACE_H

#include "instance.hpp"
#include "window/window.hpp"

namespace dry::vkw {

class vk_surface{
public:
    vk_surface(const vk_instance& instance, const wsi::window& window);

    vk_surface() = default;
    vk_surface(vk_surface&& oth) { *this = std::move(oth); }
    ~vk_surface();

    VkSurfaceKHR handle() const { return _surface; }

    vk_surface& operator=(vk_surface&&);

private:
    const vk_instance* _instance = nullptr;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
};

}

#endif
