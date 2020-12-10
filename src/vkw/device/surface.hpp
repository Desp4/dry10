#pragma once

#include "instance.hpp"
#include "../../window/window.hpp"

namespace vkw
{
    class Surface : public Movable<Surface>
    {
    public:
        using Movable<Surface>::operator=;

        Surface() = default;
        Surface(Surface&&) = default;
        Surface(const Instance* instance, wsi::NativeHandle handle);
        ~Surface();

        VkSurfaceKHR surface() const;

    private:
        NullablePtr<const Instance> _instance;
        VkHandle<VkSurfaceKHR> _surface;
    };
}