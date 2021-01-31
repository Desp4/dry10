#include "dryprogram.hpp"

namespace gr
{
    DryProgram::DryProgram() :
        DryProgram(DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT)
    {
    }

    DryProgram::DryProgram(uint32_t width, uint32_t height) :
        _window(width, height),
        _grInstance(_window.windowHandle()),
        _resManager(_grInstance)
    {
    }

    void DryProgram::beginRender()
    {
        onStart();
        while (!_window.shouldClose())
        {
            _window.pollEvents();

            _resManager.advanceFrame();
            update();
            _resManager.submitFrame();
        }
        _resManager.waitOnDevice();
    }
}