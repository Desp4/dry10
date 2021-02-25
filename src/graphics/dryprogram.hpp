#pragma once

#include "resman.hpp"

namespace dry::gr {

class dry_program {
public:
    constexpr static uint32_t DEFAULT_WIN_WIDTH = 720;
    constexpr static uint32_t DEFAULT_WIN_HEIGHT = 480;

    dry_program();
    dry_program(uint32_t width, uint32_t height);
    virtual ~dry_program() = default;

    void begin_render();

protected:
    virtual void on_start() = 0;
    virtual void update() = 0;

    wsi::window _window;
private:
    graphics_instance _gr_instance;
protected:
    resource_manager _res_man;
};

}