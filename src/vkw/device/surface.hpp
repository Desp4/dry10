#pragma once

#include "instance.hpp"
#include "window/window.hpp"

namespace dry::vkw {

class surface : public movable<surface> {
public:
    using movable<surface>::operator=;

    surface() = default;
    surface(surface&&) = default;
    surface(const instance* inst, wsi::native_handle handle);
    ~surface();

    const VkSurfaceKHR& vk_surface() const {
        return _surface;
    }

private:
    nullable_ptr<const instance> _instance;
    vk_handle<VkSurfaceKHR> _surface;
};

}