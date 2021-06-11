#include <glm/gtc/matrix_transform.hpp>

#include "graphics/renderer.hpp"
#include "asset/asset_resource_adapter.hpp"

using namespace dry;

constexpr u32_t circle_count = 3;
constexpr u32_t circle_obj_count = 10;
constexpr f32_t angle_offset = 2 * glm::pi<f32_t>() / circle_obj_count;

constexpr u32_t delete_rend_start_us = 300000;
constexpr u32_t new_rend_start_us = 0;
constexpr u32_t mutate_circle_us = 1000000;

constexpr std::array<std::string_view, 3> shader_names{ "def_tex", "def_inv", "def_pos" };
constexpr std::array<std::string_view, 3> mesh_names{ "volga", "viking_room", "granite" };
constexpr std::array mesh_scales{ 10.0f, 0.2f, 10.0f };

struct rend_pair {
    asset::asset_resource_adapter::renderable_type handle;
    model_transform transform;
    glm::mat4 pos;
    glm::mat4 scale;
};
struct object_circle {
    f32_t distance;
    f32_t anglular_offset;
    f32_t angular_speed;
    f32_t direction;
    f32_t radius;
    std::array<rend_pair, circle_obj_count> rends;
};

auto create_meshes(asset::asset_registry& reg, asset::asset_resource_adapter& adapter) {
    std::array<asset::asset_resource_adapter::index_type, mesh_names.size()> meshes;
    for (auto i = 0u; i < meshes.size(); ++i) {
        const auto mesh_asset_h = reg.get<asset::mesh_asset>(mesh_names[i].data()).hash;
        meshes[i] = adapter.get_resource_index<asset::mesh_asset>(mesh_asset_h);
    }
    return meshes;
}

auto create_materials(asset::asset_registry& reg, asset::asset_resource_adapter& adapter) {
    std::array<std::array<asset::asset_resource_adapter::material_type, mesh_names.size()>, shader_names.size()> materials;
    for (auto i = 0u; i < materials.size(); ++i) {
        const auto def_h = reg.get<asset::shader_asset>(shader_names[i].data()).hash;

        if (shader_names[i] == "def_pos") {
            const asset::material_source material_src{ .shader = def_h };
            const auto material = reg.create<asset::material_asset>(material_src).hash;
            const auto material_h = adapter.get_resource_index<asset::material_asset>(material);

            for (auto& elem : materials[i]) {
                elem = material_h;
            }
        }
        else {
            for (auto j = 0u; j < materials[i].size(); ++j) {
                const auto tex_asset_h = reg.get<asset::texture_asset>(mesh_names[j].data()).hash;

                const asset::material_source material_src{ .shader = def_h,  .textures{ tex_asset_h } };
                const auto material = reg.create<asset::material_asset>(material_src).hash;
                materials[i][j] = adapter.get_resource_index<asset::material_asset>(material);
            }
        }
    }
    return materials;
}

glm::mat4 scale_matrix(float val) {
    return glm::scale(glm::mat4{ 1.0 }, glm::vec3(val, val, val));
};

int main() {
    // init
    wsi::window window{ 640, 640 };
    vulkan_renderer vk_rend{ window };
    asset::asset_registry asset_reg;
    asset::asset_resource_adapter res_adapter{ asset_reg };
    res_adapter.attach_renderer_resource_registry(vk_rend.resource_registry());

    const auto meshes = create_meshes(asset_reg, res_adapter);
    const auto materials = create_materials(asset_reg, res_adapter);

    // init groups
    std::array<object_circle, circle_count> obj_groups;
    for (auto i = 0u; i < obj_groups.size(); ++i) {
        obj_groups[i].distance = -100.0f - 25.0f * i;
        obj_groups[i].anglular_offset = 0.76f * i;
        obj_groups[i].angular_speed = 0.0000002f * (i + 1);
        obj_groups[i].direction = (i % 2) ? -1.0f : 1.0f;
        obj_groups[i].radius = 50.0f - 5.0f * i;
    }

    std::srand(std::time(nullptr));
    // spawn all
    for (auto& group : obj_groups) {
        for (auto& rend : group.rends) {
            u32_t mat_ind = rand() % 3;
            u32_t mesh_ind = rand() % 3;

            rend.pos = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, group.distance));
            rend.scale = scale_matrix(mesh_scales[mesh_ind]);
            rend.transform = glm::rotate(rend.pos, glm::radians(-90.0f), { 0.0f, 1.0f, 0.0f });
            rend.transform = rend.transform * rend.scale;

            rend.handle = res_adapter.create_renderable(materials[mat_ind][mesh_ind], meshes[mesh_ind]);
            res_adapter.bind_renderable_transform(rend.handle, rend.transform);
        }
    }

    u64_t t_elapsed = 0;
    u64_t t_delete = delete_rend_start_us;
    u64_t t_new = new_rend_start_us;

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

        if (t_delete >= mutate_circle_us) {
            for (auto& group : obj_groups) {
                res_adapter.destroy_renderable(group.rends[i_delete].handle);
            }
            i_delete = (i_delete + 1) % circle_obj_count;
            t_delete = 0;
        }

        if (t_new >= mutate_circle_us) {
            for (auto& group : obj_groups) {
                u32_t mat_ind = rand() % materials.size();
                u32_t mesh_ind = rand() % meshes.size();

                auto& rend = group.rends[i_new];
                rend.scale = scale_matrix(mesh_scales[mesh_ind]);
                rend.handle = res_adapter.create_renderable(materials[mat_ind][mesh_ind], meshes[mesh_ind]);
                res_adapter.bind_renderable_transform(rend.handle, rend.transform);
            }

            i_new = (i_new + 1) % circle_obj_count;
            t_new = 0;
        }

        for (auto& group : obj_groups) {
            for (auto i = 0u; i < group.rends.size(); ++i) {
                auto& rend = group.rends[i];
                const f32_t angle = group.direction * (t_elapsed * group.angular_speed + angle_offset * i + group.anglular_offset);

                rend.transform = glm::translate(rend.pos,
                    glm::vec3(group.radius * std::cosf(angle), group.radius * std::sinf(angle), 0.0f)
                );
                rend.transform = glm::rotate(rend.transform, glm::radians(-90.0f), { 0.0f, 1.0f, 0.0f });
                rend.transform = rend.transform * rend.scale;
            }
        }

        vk_rend.submit_frame();
    }
    return 0;
}