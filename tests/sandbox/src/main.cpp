#include <glm/gtc/matrix_transform.hpp>

#include "graphics/renderer.hpp"
#include "asset/asset_resource_adapter.hpp"

using namespace dry;

int main() {
    const glm::mat4 def_scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2f, 0.2f, 0.2f));

    wsi::window window{ 640, 640 };
    vulkan_renderer vk_rend{ window };
    asset::asset_registry asset_reg;
    asset::asset_resource_adapter res_adapter{ asset_reg };
    res_adapter.attach_renderer_resource_registry(vk_rend.resource_registry());

    const auto mesh_asset_h = asset_reg.get<asset::mesh_asset>("viking_room").hash;
    const auto tex_asset_h = asset_reg.get<asset::texture_asset>("viking_room").hash;
    const auto shader_asset_h = asset_reg.get<asset::shader_asset>("def").hash;

    const asset::material_source material_src{ .shader = shader_asset_h, .textures{tex_asset_h} };
    const auto material = asset_reg.create<asset::material_asset>(material_src).hash;

    const auto render_material = res_adapter.get_resource_index<asset::material_asset>(material);
    const auto render_mesh = res_adapter.get_resource_index<asset::mesh_asset>(mesh_asset_h);
    
    model_transform transform1 = glm::translate(glm::mat4{ 1.0f }, glm::vec3(15.0f, 0.0f, -55.0f));
    {
        transform1 = glm::rotate(transform1, glm::radians(-90.0f), { 0.0f, 1.0f, 0.0f });
        transform1 = transform1 * def_scale;

        const auto renderable_h = res_adapter.create_renderable(render_material, render_mesh);
        res_adapter.bind_renderable_transform(renderable_h, transform1);
    }

    model_transform transform2 = glm::translate(glm::mat4{ 1.0f }, glm::vec3(-15.0f, 0.0f, -55.0f));
    {
        transform2 = glm::rotate(transform2, glm::radians(-90.0f), { 0.0f, 1.0f, 0.0f });
        transform2 = transform2 * def_scale;

        const auto renderable_h = res_adapter.create_renderable(render_material, render_mesh);
        res_adapter.bind_renderable_transform(renderable_h, transform2);
    }

    while (!window.should_close()) {
        window.poll_events();

        vk_rend.submit_frame();
    }

    return 0;
}