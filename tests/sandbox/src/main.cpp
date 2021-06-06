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

    struct rend_pair {
        asset::asset_resource_adapter::renderable_type handle;
        model_transform transform;
        glm::mat4 pos;
    };

    std::array<rend_pair, 10> rends;
    constexpr f32_t angle_offset = 2 * glm::pi<f32_t>() / rends.size();
    constexpr f32_t angular_speed = 0.0000005f;
    for (auto& rend : rends) {
        rend.pos = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, -150.0f));
        rend.transform = glm::rotate(rend.pos, glm::radians(-90.0f), { 0.0f, 1.0f, 0.0f });
        rend.transform = rend.transform * def_scale;

        rend.handle = res_adapter.create_renderable(render_material, render_mesh);
        res_adapter.bind_renderable_transform(rend.handle, rend.transform);
    }

    u64_t t_elapsed = 0;
    u64_t t_delete = 400000;
    u64_t t_new = 0;

    u32_t i_delete = 0;
    u32_t i_new = 0;

    auto t0 = std::chrono::steady_clock::now();
    while (!window.should_close()) {
        window.poll_events();

        const auto t1 = std::chrono::steady_clock::now();
        const auto dt = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        t0 = t1;

        t_elapsed += dt;
        t_delete += dt;
        t_new += dt;

        if (t_delete >= 1000000) {
            res_adapter.destroy_renderable(rends[i_delete].handle);
            i_delete = (i_delete + 1) % rends.size();
            t_delete = 0;
        }
        if (t_new >= 1000000) {
            auto& rend = rends[i_new];
            rend.handle = res_adapter.create_renderable(render_material, render_mesh);
            res_adapter.bind_renderable_transform(rend.handle, rend.transform);

            i_new = (i_new + 1) % rends.size();
            t_new = 0;
        }

        for (auto i = 0u; i < rends.size(); ++i) {
            auto& rend = rends[i];
            const f32_t angle = t_elapsed * angular_speed + angle_offset * i;

            rend.transform = glm::translate(rend.pos,
                glm::vec3(50 * std::cosf(angle), 50 * std::sinf(angle), 0.0f)
            );
            rend.transform = glm::rotate(rend.transform, glm::radians(-90.0f), { 0.0f, 1.0f, 0.0f });
            rend.transform = rend.transform * def_scale;
        }

        vk_rend.submit_frame();
    }

    return 0;
}