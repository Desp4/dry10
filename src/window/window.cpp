#ifdef _WIN32
 #define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include "window.hpp"

#include <GLFW/glfw3native.h>

namespace dry::wsi {

window::window(uint32_t width, uint32_t height) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(width, height, "dry1", nullptr, nullptr);
}

window::~window() {
    glfwDestroyWindow(_window);
}

bool window::should_close() const {
    return glfwWindowShouldClose(_window);
}

void window::poll_events() const {
    glfwPollEvents();
}

native_handle window::window_handle() const {
    return glfwGetWin32Window(_window);
}

}