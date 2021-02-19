#pragma once

#include "vkw/vkw.hpp"
#include "window/window.hpp"

namespace dry::vkw {

class surface : public movable<surface> {
public:
    using movable<surface>::operator=;

    surface() = default;
    surface(surface&&) = default;
    surface(wsi::native_handle handle);
    ~surface();

    const VkSurfaceKHR& vk_surface() const {
        return _surface;
    }

private:
    vk_handle<VkSurfaceKHR> _surface;
};

}