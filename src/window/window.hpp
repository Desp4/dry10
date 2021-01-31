#pragma once

#ifdef _WIN32
  #include <windows.h>
#endif

#ifdef _WIN32
  #define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "util/util.hpp"

namespace wsi
{
    struct GLFWDummy
    {
        GLFWDummy()
        {
            glfwInit();
        }

        ~GLFWDummy()
        {
            glfwTerminate();
        }
    };

#ifdef _WIN32
    using NativeHandle = HWND;
#endif

    class Window : public util::Movable<Window>
    {
    public:
        using util::Movable<Window>::operator=;

        Window() = default;
        Window(Window&&) = default;
        Window(uint32_t width, uint32_t height);
        ~Window();

        bool shouldClose() const;
        void pollEvents() const;
        NativeHandle windowHandle() const;
        GLFWwindow* window() const { return _window; }

    private:
        util::NullablePtr<GLFWwindow> _window;
    };
}