#include "dry_program.hpp"

namespace dry {

renderable::~renderable() {
    if (_renderer != nullptr) {
        _renderer->destroy_renderable(_handle);
    }
}

renderable& renderable::operator=(renderable&& oth) {
    // destroy
    if (_renderer != nullptr) {
        _renderer->destroy_renderable(_handle);
    }
    // move
    trans = oth.trans;
    _renderer = oth._renderer;
    _handle = oth._handle;
    // null
    oth._renderer = nullptr;
    return *this;
}

void renderable::commit_transform() {
    const auto rend_transform = object_transform{ .model =
        glm::translate(glm::mat4{ 1.0f }, trans.position) * glm::scale(glm::mat4{ 1.0f }, trans.scale) * glm::toMat4(trans.rotation)
    };
    _renderer->update_renderable_transform(_handle, rend_transform);
}

dry_program::dry_program(u32_t w, u32_t h) :
    _window{ w, h },
    _renderer{ _window },
    _asset_reg{},
    _resource_adapter{ _asset_reg }
{
    _resource_adapter.attach_renderer(_renderer);
    glfwSetWindowUserPointer(_window.handle(), this);

    glfwSetInputMode(_window.handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(_window.handle(), key_callback);
    glfwSetCursorPosCallback(_window.handle(), mouse_callback);
    glfwSetScrollCallback(_window.handle(), wheel_callback);

    _camera.aspect = static_cast<f32_t>(w) / h;

    _mouse_prev_x = static_cast<f32_t>(w) / 2;
    _mouse_prev_y = static_cast<f32_t>(h) / 2;
}

void dry_program::render_loop() {
    _elapsed_time = 0.0;
    _t0 = std::chrono::steady_clock::now();

    while (!_window.should_close()) {
        _window.poll_events();

        // calculate time
        {
            const auto t1 = std::chrono::steady_clock::now();
            _delta_time = std::chrono::duration<f64_t>(t1 - _t0).count();
            _elapsed_time += _delta_time;
            _t0 = t1;
        }

        if (!update()) {
            break;
        }

        update_camera();

        // clear input
        _mouse_input.dx = 0;
        _mouse_input.dy = 0;
        _wheel_delta = 0;

        _renderer.submit_frame();
    }
}

renderable dry_program::create_renderable(res_index mesh, res_index material) {
    renderable rend;
    rend._handle = _renderer.create_renderable(material, mesh);
    rend._renderer = &_renderer;
    rend.trans.scale = { 1, 1, 1 };
    rend.trans.position = { 0, 0, 0 };
    rend.trans.rotation = { 0, 0, 0, 1 };

    return rend;
}

renderable dry_program::create_renderable(const std::string& mesh, res_index material) {
    return create_renderable(_resource_adapter.get_resource_index<asset::mesh_asset>(_asset_reg.get<asset::mesh_asset>(mesh).hash), material);
}

void dry_program::update_camera() {
    const auto dir = _camera.trans.position + glm::normalize(glm::rotate(_camera.trans.rotation, { 0, 0, 1 }));
    const auto up = glm::rotate(_camera.trans.rotation, { 0, 1, 0 });
    
    camera_transform cam;
    cam.view = glm::lookAt(_camera.trans.position, dir, up);
    cam.proj = glm::perspective(_camera.fov, _camera.aspect, 0.1f, 8000.0f);
    cam.proj[1][1] *= -1;
    cam.viewproj = cam.proj * cam.view;
 
    _renderer.update_camera_transform(cam);
}

void dry_program::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    dry_program& program = *reinterpret_cast<dry_program*>(glfwGetWindowUserPointer(window));

    const bool key_value = (action != GLFW_RELEASE);
    program._keyboard_input.set(key, key_value);
}

void dry_program::wheel_callback(GLFWwindow* window, double x, double y) {
    dry_program& program = *reinterpret_cast<dry_program*>(glfwGetWindowUserPointer(window));
    
    program._wheel_delta = static_cast<f32_t>(y);
}

void dry_program::mouse_callback(GLFWwindow* window, double x, double y) {
    dry_program& program = *reinterpret_cast<dry_program*>(glfwGetWindowUserPointer(window));

    // TODO : to plug sudden jumps, something nicer eh?
    static bool first_call = true;
    if (first_call) {
        program._mouse_prev_x = static_cast<f32_t>(x);
        program._mouse_prev_y = static_cast<f32_t>(y);
        first_call = false;
        return;
    }

    program._mouse_input.dx = static_cast<f32_t>(x - program._mouse_prev_x);
    program._mouse_input.dy = static_cast<f32_t>(y - program._mouse_prev_y);

    program._mouse_prev_x = static_cast<f32_t>(x);
    program._mouse_prev_y = static_cast<f32_t>(y);
}

}