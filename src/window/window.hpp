#pragma once

#ifndef DRY_WINDOW_H
#define DRY_WINDOW_H

#include <string_view>

#include <GLFW/glfw3.h>

#include "util/num.hpp"

namespace dry::wsi {

class window {
public:
    window(u32_t width, u32_t height) : window{ width, height, "dry1 instance" } {}
    window(u32_t width, u32_t height, std::string_view title);

    window();
    window(window&& oth) { *this = std::move(oth); }
    ~window();

    bool should_close() const;
    void poll_events() const;
    void set_title(std::string_view title);

    GLFWwindow* handle() const { return _window; }

    window& operator=(window&&);

private:
    GLFWwindow* _window = nullptr;
};

}

#endif
