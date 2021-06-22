#include "window.hpp"

namespace dry::wsi {

// dummy call on every window creation, not a big deal
void init_glfw() {
    struct glfw_dummy {
        glfw_dummy() {
            glfwInit();
        }
        ~glfw_dummy() {
            glfwTerminate();
        }
    };
    const static glfw_dummy dummy{};
}

window::window() noexcept {
    init_glfw();
}

window::window(u32_t width, u32_t height, std::string_view title) noexcept : window{} {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
}

window::~window() {
    glfwDestroyWindow(_window); // null is checked inside, ok
}

bool window::should_close() const {
    return glfwWindowShouldClose(_window);
}

void window::poll_events() const {
    glfwPollEvents();
}

void window::set_title(std::string_view title) {
    glfwSetWindowTitle(_window, title.data());
}

window& window::operator=(window&& oth) noexcept {
    // destroy
    glfwDestroyWindow(_window);
    // move
    _window = oth._window;
    // null
    oth._window = nullptr;
    return *this;
}

}
