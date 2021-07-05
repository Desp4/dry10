#include <random>

#include "engine/dry_program.hpp"

using namespace dry;

class textured_material : public material_base {
public:
    vulkan_renderer::resource_id texture;

    textured_material(vulkan_renderer::resource_id tex) : texture{ tex } {}

    void write_material_info(std::byte* dst) const override {
        *reinterpret_cast<decltype(texture)*>(dst) = texture;
    }
};

class empty_material : public material_base {
public:
    void write_material_info(std::byte* dst) const override {}
};

class orbitals : public dry_program {
public:
    orbitals();
    ~orbitals();

    bool update() override;

private:
    // assets
    static constexpr std::array _shader_names{ "unlit_tex", "unlit_tex_inv", "unlit_normal", "lit_tex"};
    static constexpr std::array _mesh_names{ "volga", "viking_room", "granite" };
    static constexpr std::array _texture_names = _mesh_names;
    static constexpr std::array _mesh_scaling_factors{ 1.0f, 0.02f , 1.0f };
    // orbit
    static constexpr u32_t _orbit_count = 20;
    static constexpr u32_t _orbit_object_count = 200;
    static constexpr f32_t _orbit_object_angle_delta = 2 * glm::pi<f32_t>() / _orbit_object_count;
    // spawn
    static constexpr f64_t _spawn_period = 1;
    static constexpr f64_t _spawn_delete_start = 0.5;
    static constexpr f64_t _spawn_create_start = 0.0;
    // speed
    static constexpr f32_t _fov_speed = 0.05f;
    static constexpr f32_t _camera_speed = 75.0f;
    static constexpr f32_t _camera_sensetivity = 0.01f;

    // shader
    struct light_source {
        alignas(16) glm::vec3 position;
        alignas(16) glm::vec3 color;
    };

    static constexpr light_source _lit_light_source{ .position{ 0, 0, -200 }, .color{ 1.0f, 0.2f, 1.0f } };
    struct object_orbit {
        std::vector<renderable> objects;
        f32_t distance;
        f32_t anglular_offset;
        f32_t angular_speed;
        f32_t direction;
        f32_t radius;
    };

    struct {
        std::random_device device;
        std::mt19937 rng{ device() };
        std::uniform_int_distribution<u32_t> mesh_distr{ 0, static_cast<u32_t>(_mesh_names.size() - 1) };
        std::uniform_int_distribution<u32_t> shader_distr{ 0, static_cast<u32_t>(_shader_names.size() - 1) };
    } _rng;

    std::array<std::array<res_index, _texture_names.size()>, _shader_names.size()> _materials;
    std::array<res_index, _mesh_names.size()> _meshes;

    std::vector<object_orbit> _orbits;

    f64_t _delete_timer = _spawn_delete_start;
    f64_t _create_timer = _spawn_create_start;

    f32_t _camera_pitch = 0;
    f32_t _camera_yaw = 0;

    std::vector<f64_t> _frame_times;
    u32_t _frame_time_ind = 0;
};

orbitals::orbitals() : dry_program{} {
    // create materials
    for (auto i = 0u; i < _shader_names.size(); ++i) {
        // pos shader has different material
        const auto shader = get_asset<asset::shader_asset>(_shader_names[i]).hash;
        if (i == 2) {
            empty_material material;
            for (auto j = 0u; j < _texture_names.size(); ++j) {               
                _materials[i][j] = construct_resource<asset::material_asset, decltype(material)>(shader, material);
            }

        } else {
            for (auto j = 0u; j < _texture_names.size(); ++j) {
                textured_material material{ create_resource<asset::texture_asset>(_texture_names[j]) };
                _materials[i][j] = construct_resource<asset::material_asset, decltype(material)>(shader, material);
            }
        }       
    }
    // upload uniform for lit
    write_shader_data(create_resource<asset::shader_asset>(_shader_names[3]), 1, _lit_light_source);

    // create meshes
    for (auto i = 0u; i < _mesh_names.size(); ++i) {
        _meshes[i] = create_resource<asset::mesh_asset>(_mesh_names[i]);
    }

    // create orbits
    _orbits.resize(_orbit_count);
    for (auto i = 0u; i < _orbits.size(); ++i) {
        _orbits[i].objects.reserve(_orbit_object_count);
        // hardcoded settings for generation
        _orbits[i].distance = -150.0f - 25.0f * i;
        _orbits[i].anglular_offset = 0.76f * i;
        _orbits[i].angular_speed = 0.02f + i * 0.005f;
        _orbits[i].direction = (i % 2) ? -1.0f : 1.0f;
        _orbits[i].radius = 100.0f - 2.0f * i;

        for (auto j = 0u; j < _orbit_object_count; ++j) {
            const u32_t shader_ind = _rng.shader_distr(_rng.rng);
            const u32_t mesh_ind = _rng.mesh_distr(_rng.rng);

            auto object = create_renderable(_meshes[mesh_ind], _materials[shader_ind][mesh_ind]);

            const f32_t scaling_factor = _mesh_scaling_factors[mesh_ind];
            object.trans.position = { 0, 0, _orbits[i].distance };
            object.trans.scale = { scaling_factor, scaling_factor, scaling_factor };
            object.trans.rotation = glm::rotate(object.trans.rotation, { 0, glm::radians(180.0f), 0 });

            object.commit_transform();
            _orbits[i].objects.push_back(std::move(object));
        }
    }

    _camera_yaw = glm::radians(180.0f);

    _frame_times.resize(512, 0);
}

orbitals::~orbitals() {
    f64_t total = 0;
    for (auto time : _frame_times) {
        total += time;
    }
    const f32_t frame_time = static_cast<f32_t>(total / _frame_times.size());
    printf("average over %zi frames %fms (%ffps)\n", _frame_times.size(), frame_time * 1000, 1.0f / frame_time);
}

bool orbitals::update() {
    _frame_times[_frame_time_ind] = _delta_time;
    _frame_time_ind = (_frame_time_ind + 1) % _frame_times.size();

    _delete_timer += _delta_time;
    _create_timer += _delta_time;

    _camera.fov += _wheel_delta * _fov_speed;
    _camera.fov = std::clamp(_camera.fov, glm::radians(5.0f), glm::radians(120.0f));

    {
        const auto camera_front = glm::normalize(glm::rotate(_camera.trans.rotation, { 0, 0, 1 }));
        const auto camera_side = glm::normalize(glm::cross(camera_front, { 0, 1, 0 }));
        // glm come on
        _camera.trans.position += static_cast<f32_t>(_keyboard_axis.x * _camera_speed * _delta_time) * camera_front;
        _camera.trans.position += static_cast<f32_t>(_keyboard_axis.y * _camera_speed * _delta_time) * camera_side;

        // suboptimal rotation, learn math pls
        constexpr f32_t pitch_limit = glm::radians(89.0f);
        _camera_pitch += _mouse_axis.dy * _camera_sensetivity;
        _camera_pitch = std::clamp(_camera_pitch, -pitch_limit, pitch_limit);
        _camera_yaw += -_mouse_axis.dx * _camera_sensetivity;

        const glm::quat pitch = glm::angleAxis(_camera_pitch, glm::vec3{ 1, 0, 0 });
        const glm::quat yaw = glm::angleAxis(_camera_yaw, glm::vec3{ 0, 1, 0 });

        _camera.trans.rotation = yaw * pitch;
    }


    if (_delete_timer >= _spawn_period) {
        for (auto& orbit : _orbits) {
            orbit.objects.pop_back();
        }
        _delete_timer = 0;
    }

    if (_create_timer >= _spawn_period) {
        for (auto& orbit : _orbits) {
            const u32_t shader_ind = _rng.shader_distr(_rng.rng);
            const u32_t mesh_ind = _rng.mesh_distr(_rng.rng);

            auto object = create_renderable(_meshes[mesh_ind], _materials[shader_ind][mesh_ind]);

            const f32_t scaling_factor = _mesh_scaling_factors[mesh_ind];
            object.trans.position = { 0, 0, orbit.distance };
            object.trans.scale = { scaling_factor, scaling_factor, scaling_factor };
            object.trans.rotation = glm::rotate(object.trans.rotation, { 0, glm::radians(180.0f), 0 });

            orbit.objects.push_back(std::move(object));
        }

        _create_timer = 0;
    }


    for (auto& orbit : _orbits) {
        for (auto i = 0u; i < orbit.objects.size(); ++i) {
            auto& object = orbit.objects[i];

            const f32_t angle = orbit.direction * 
                (static_cast<f32_t>(_elapsed_time) * orbit.angular_speed + _orbit_object_angle_delta * i + orbit.anglular_offset);
            object.trans.position = { orbit.radius * std::cosf(angle), orbit.radius * std::sinf(angle), orbit.distance };

            object.commit_transform();
        }
    }
    return true;
}


int main() {
    orbitals program;

    program.render_loop();
    return 0;
}
