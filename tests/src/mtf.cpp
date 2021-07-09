#include "common.hpp"

#include "math/geometry.hpp"

class mtf_test : public fps_dry_program {
public:
    mtf_test();

    bool update() override;

private:
    static constexpr std::array _mesh_names{ "volga", "viking_room", "granite" };
    static constexpr std::array _mesh_scaling_factors{ 1.0f, 0.02f , 1.0f };

    static constexpr f32_t _camera_speed = 15.0f;
    static constexpr f32_t _camera_sensetivity = 0.01f;

    std::vector<renderable> _renderables;
    std::vector<renderable> _circles;
};


mtf_test::mtf_test() : fps_dry_program{} {
    // create quad
    constexpr u32_t circle_resolution = 128;
    constexpr f32_t circle_angle_delta = 2.0f * glm::pi<f32_t>() / circle_resolution;

    // generate circle mesh
    asset::mesh_source circle_mesh_src;
    circle_mesh_src.vertices.resize(circle_resolution);
    circle_mesh_src.indices.resize(circle_resolution);
    for (auto i = 0u; i < circle_resolution; ++i) {
        circle_mesh_src.vertices[i].pos = { std::sinf(circle_angle_delta * i), std::cosf(circle_angle_delta * i), 0.0f };
        circle_mesh_src.indices[i] = i;
    }

    const auto circle_mesh_h = construct_resource<asset::mesh_asset>(std::move(circle_mesh_src));

    const auto mesh_shader = get_asset<asset::shader_asset>("unlit_normal").hash;
    // abuse the fact that asset reg returns mutable references, TODO : don't return mutable refs please
    auto& circle_shader = get_asset<asset::shader_asset>("plain_color");
    circle_shader.create_ctx.fill_mode = VK_POLYGON_MODE_LINE;
    empty_material empty_mat;

    const auto mesh_material = construct_resource<asset::material_asset, decltype(empty_mat)>(mesh_shader, empty_mat);
    const auto circle_material = construct_resource<asset::material_asset, decltype(empty_mat)>(circle_shader.hash, empty_mat);

    for (auto i = 0u; i < _mesh_names.size(); ++i) {
        const auto& mesh_asset = get_asset<asset::mesh_asset>(_mesh_names[i]);
        const auto mesh_h = create_resource<asset::mesh_asset>(mesh_asset.hash);

        auto rend = create_renderable(mesh_h, mesh_material);

        const f32_t scaling_factor = _mesh_scaling_factors[i];
        rend.trans.position = { 0, 0, 4.0f * i };
        rend.trans.scale = { scaling_factor, scaling_factor, scaling_factor };
        rend.trans.rotation = glm::rotate(rend.trans.rotation, { 0, 0, 0 });

        // create quad
        auto circle_rend = create_renderable(circle_mesh_h, circle_material);

        std::vector<glm::vec3> points;
        points.reserve(mesh_asset.vertices.size());
        for (const auto& vertex : mesh_asset.vertices) {
            points.push_back(vertex.pos);
        }

        const auto sphere = math::minimal_bounding_sphere(std::move(points));
        const glm::vec3 scaled_pos = sphere.pos * scaling_factor;
        const f32_t circle_scaling = scaling_factor * sphere.radius;


        circle_rend.trans.position = rend.trans.position + scaled_pos;
        circle_rend.trans.scale = { circle_scaling, circle_scaling, circle_scaling };
        circle_rend.trans.rotation = glm::rotate(circle_rend.trans.rotation, { 0, glm::radians(90.0f), 0 });

        rend.commit_transform();
        circle_rend.commit_transform();

        _renderables.push_back(std::move(rend));
        _circles.push_back(std::move(circle_rend));
    }
}

bool mtf_test::update() {
    update_camera(_camera_speed, _camera_sensetivity);

    for (auto& circle : _circles) {
        circle.trans.rotation = glm::quatLookAt(
            glm::normalize(_camera.trans.position - circle.trans.position),
            { 0, 1, 0 }
        );
        circle.commit_transform();
    }

    return true;
}

int main() {
    mtf_test program;

    program.render_loop();
    return 0;
}