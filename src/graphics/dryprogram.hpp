#pragma once

#include "core/resourcemanager.hpp"
#include "asset/assetreg.hpp"

namespace gr
{
    class DryProgram
    {
    public:
        constexpr static uint32_t DEFAULT_WIN_WIDTH  = 720;
        constexpr static uint32_t DEFAULT_WIN_HEIGHT = 480;

        DryProgram();
        DryProgram(uint32_t width, uint32_t height);
        virtual ~DryProgram() = default;

        void beginRender();

    private:
        wsi::Window _window;
        core::GraphicsInstance _grInstance;

    protected:
        virtual void onStart() = 0;
        virtual void update() = 0;

        core::ResourceManager _resManager;
        asset::AssetRegistry _assetReg;
    };
}