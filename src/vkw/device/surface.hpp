#pragma once

#ifdef _WIN32
  #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "instance.hpp"
#include "window/window.hpp"

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

        const VkHandle<VkSurfaceKHR>& surface() const { return _surface; }

    private:
        NullablePtr<const Instance> _instance;
        VkHandle<VkSurfaceKHR> _surface;
    };
}