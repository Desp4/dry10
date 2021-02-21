#ifdef _WIN32
 #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "surface.hpp"
#include "instance.hpp"

namespace dry::vkw {

surface::surface(const instance* inst, wsi::native_handle handle) :
    _instance(inst)
{
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR surface_info{};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hinstance = GetModuleHandle(nullptr);
    surface_info.hwnd = static_cast<HWND>(handle);

    vkCreateWin32SurfaceKHR(_instance->vk_instance(), &surface_info, NULL_ALLOC, &_surface);
#endif
}

surface::~surface() {
    if (_instance) {
        vkDestroySurfaceKHR(_instance->vk_instance(), _surface, NULL_ALLOC);
    }   
}

}