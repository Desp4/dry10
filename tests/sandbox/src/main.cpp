#include "graphics/renderer.hpp"
#include "asset/assetreg.hpp"
#include "util/fs.hpp"

using namespace dry;

int main() {
    asset::asset_registry asset_reg;
    asset_reg.load_archive(g_exe_dir.string() + "/assets.zip");

    const auto& mesh_asset = asset_reg.get<asset::mesh_asset>("viking_room");
    const auto* tex_asset = &asset_reg.get<asset::texture_asset>("viking_room");
    const auto* shader_asset = &asset_reg.get<asset::shader_asset>("def");

    const material_asset material{ .shader = shader_asset, .textures{tex_asset} };

    wsi::window window{ 640, 640 };
    vulkan_renderer vk_rend{ window };
    vk_rend.create_renderable(mesh_asset, material);

    while (!window.should_close()) {
        window.poll_events();

        vk_rend.submit_frame();
    }

    return 0;
}