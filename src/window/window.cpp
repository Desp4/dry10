#include "window.hpp"

namespace wsi
{
    Window::Window(uint32_t width, uint32_t height)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        _window = glfwCreateWindow(width, height, "dry1", nullptr, nullptr);       
    }

    Window::~Window()
    {
        glfwDestroyWindow(_window);
    }

    bool Window::shouldClose() const
    {
        return glfwWindowShouldClose(_window);
    }

    void Window::pollEvents() const
    {
        glfwPollEvents();
    }

    NativeHandle Window::windowHandle() const
    {
        return glfwGetWin32Window(_window);
    }
}