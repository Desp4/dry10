#include "surface.hpp"
static auto f = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
namespace vkw
{
    Surface::Surface(const Instance* instance, wsi::NativeHandle handle) :
        _instance(instance)
    {
#ifdef _WIN32
        VkWin32SurfaceCreateInfoKHR surfaceInfo{};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.hinstance = GetModuleHandle(nullptr);
        surfaceInfo.hwnd = handle;

        vkCreateWin32SurfaceKHR(_instance->instance(), &surfaceInfo, NULL_ALLOC, &_surface);
#endif
    }

    Surface::~Surface()
    {
        if (_instance) vkDestroySurfaceKHR(_instance->instance(), _surface, NULL_ALLOC);
    }
}