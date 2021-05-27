#include "surface.hpp"

namespace dry::vkw {

vk_surface::vk_surface(const vk_instance& instance, const wsi::window& window) :
    _instance(&instance)
{
    glfwCreateWindowSurface(instance.handle(), window.handle(), null_alloc, &_surface);
}

vk_surface::~vk_surface() {
    if (_instance) {
        vkDestroySurfaceKHR(_instance->handle(), _surface, null_alloc);
    }   
}

vk_surface& vk_surface::operator=(vk_surface&& oth) {
    // destroy
    if (_instance) {
        vkDestroySurfaceKHR(_instance->handle(), _surface, null_alloc);
    }
    // move
    _instance = oth._instance;
    _surface = oth._surface;
    // null
    oth._instance = nullptr;
    return *this;
}

}