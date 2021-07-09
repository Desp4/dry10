#pragma once

#ifndef DRYC_TESTS
#define DRYC_TESTS

#include "engine/dry_program.hpp"

using namespace dry;

class empty_material : public material_base {
public:
    void write_material_info(std::byte* dst) const override {}
};

// NOTE : don't like inheritance as the means of extensions, can't think of anything better atm
class fps_dry_program : public dry_program {
protected:
    inline void update_camera(f32_t movement_sens, f32_t rotation_sens);

    f32_t _camera_pitch = 0;
    f32_t _camera_yaw = 0;
};

inline void fps_dry_program::update_camera(f32_t movement_sens, f32_t rotation_sens) {
    const auto camera_front = glm::normalize(glm::rotate(_camera.trans.rotation, { 0, 0, 1 }));
    const auto camera_side = glm::normalize(glm::cross(camera_front, { 0, 1, 0 }));

    const f32_t mouse_x = static_cast<f32_t>(_keyboard_input.test(GLFW_KEY_W) - _keyboard_input.test(GLFW_KEY_S));
    const f32_t mouse_y = static_cast<f32_t>(_keyboard_input.test(GLFW_KEY_D) - _keyboard_input.test(GLFW_KEY_A));

    _camera.trans.position += static_cast<f32_t>(mouse_x * movement_sens * _delta_time) * camera_front;
    _camera.trans.position += static_cast<f32_t>(mouse_y * movement_sens * _delta_time) * camera_side;

    constexpr f32_t pitch_limit = glm::radians(89.0f);
    _camera_pitch += _mouse_input.dy * rotation_sens;
    _camera_pitch = std::clamp(_camera_pitch, -pitch_limit, pitch_limit);
    _camera_yaw += -_mouse_input.dx * rotation_sens;

    const glm::quat pitch = glm::angleAxis(_camera_pitch, glm::vec3{ 1, 0, 0 });
    const glm::quat yaw = glm::angleAxis(_camera_yaw, glm::vec3{ 0, 1, 0 });

    _camera.trans.rotation = yaw * pitch;
}

#endif