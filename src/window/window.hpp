#pragma once

#include <GLFW/glfw3.h>

#include "util/util.hpp"

namespace dry::wsi {

struct gldw_dummy {
    gldw_dummy() {
        glfwInit();
    }
    ~gldw_dummy() {
        glfwTerminate();
    }
};

// opaque to not include win32 here
using native_handle = void*;

class window : public util::movable<window> {
public:
    using util::movable<window>::operator=;

    window() = default;
    window(uint32_t width, uint32_t height);
    ~window();

    bool should_close() const;
    void poll_events() const;
    native_handle window_handle() const;

private:
    util::nullable_ptr<GLFWwindow> _window;
};

}