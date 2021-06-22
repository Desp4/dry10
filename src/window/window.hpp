#pragma once

#ifndef DRY_WINDOW_WINDOW_H
#define DRY_WINDOW_WINDOW_H

#include <string_view>

#include <GLFW/glfw3.h>

#include "util/num.hpp"

namespace dry::wsi {

class window {
public:
    window(u32_t width, u32_t height) noexcept : window{ width, height, "dry1 instance" } {}
    window(u32_t width, u32_t height, std::string_view title) noexcept;

    window() noexcept;
    window(window&& oth) noexcept { *this = std::move(oth); }
    ~window();

    bool should_close() const;
    void poll_events() const;
    void set_title(std::string_view title);

    GLFWwindow* handle() const { return _window; }

    window& operator=(window&&) noexcept;

private:
    GLFWwindow* _window = nullptr;
};

}

#endif
