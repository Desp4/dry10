#ifdef _WIN32
 #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "surface.hpp"
#include "instance.hpp"

namespace dry::vkw {

surface::surface(wsi::native_handle handle) {
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR surface_info{};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hinstance = GetModuleHandle(nullptr);
    surface_info.hwnd = static_cast<HWND>(handle);

    vkCreateWin32SurfaceKHR(instance_main::instance(), &surface_info, NULL_ALLOC, &_surface);
#endif
}

surface::~surface() {
    // TODO : get rid of global instance, it serves little purpose and forces to perform this check
    // when graphics lifetime need to free a surface after deleting instance
    if (_surface) {
        vkDestroySurfaceKHR(instance_main::instance(), _surface, NULL_ALLOC);
    }   
}

}