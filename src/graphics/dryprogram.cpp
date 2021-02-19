#include "dryprogram.hpp"
#include "vkw/device/device.hpp"

namespace dry::gr {

dry_program::dry_program() :
    dry_program(DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT)
{
}

dry_program::dry_program(uint32_t width, uint32_t height) :
    _window(width, height),
    _gr_instance(_window.window_handle()),
    _res_man(_gr_instance)
{
}

void dry_program::begin_render() {
    on_start();
    while (!_window.should_close()) {
        _window.poll_events();

        _res_man.advance_frame();
        update();
        _res_man.submit_frame();
    }
    vkw::device_main::wait_on_device();
}

}