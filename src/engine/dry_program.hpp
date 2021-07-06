#pragma once

#ifndef DRY_ENGINE_DRY_PROGRAM_H
#define DRY_ENGINE_DRY_PROGRAM_H

#include <glm/gtx/quaternion.hpp>

#include "graphics/renderer.hpp"
#include "asset/asset_resource_adapter.hpp"

namespace dry {

struct transform {
    glm::vec3 position;
    glm::vec3 scale;
    glm::quat rotation;
};

class dry_program;

struct renderable {
    transform trans;

    ~renderable();
    renderable(const renderable&) = delete;
    renderable(renderable&& oth) { *this = std::move(oth); }

    void commit_transform();

    renderable& operator=(const renderable&) = delete;
    renderable& operator=(renderable&&);

private:
    friend class dry_program;

    renderable() = default;

    vulkan_renderer::renderable_id _handle;
    vulkan_renderer* _renderer = nullptr;
};

class dry_program {
public:
    using res_index = vulkan_renderer::resource_id;
    using asset_index = asset::hash_t;

    static constexpr u32_t default_win_w = 640;
    static constexpr u32_t default_win_h = 640;

    dry_program(u32_t w = default_win_w, u32_t h = default_win_h);
    virtual ~dry_program() = default;

    void render_loop();

protected:
    // returning false terminates the loop
    virtual bool update() { return true; }

    renderable create_renderable(res_index mesh, res_index material);
    renderable create_renderable(const std::string& mesh, res_index material);

    template<typename T, typename... Ts>
    asset_index create_asset(Ts&&... args);
    template<typename T>
    T& get_asset(asset_index index);
    template<typename T>
    T& get_asset(const std::string& name);

    // TODO : bad naming too: does not construct if already constructed
    template<typename T> requires (!std::is_same_v<T, asset::material_asset>)
    res_index create_resource(asset_index index);
    template<typename T, typename Mat> requires std::is_same_v<T, asset::material_asset>
    res_index create_resource(asset_index index);

    // create from name, materials not present
    template<typename T> requires (!std::is_same_v<T, asset::material_asset>)
    res_index create_resource(const std::string& name);

    // TODO : poor naming
    // create_asset + create_resource
    template<typename T, typename... Ts> requires (!std::is_same_v<T, asset::material_asset>)
    res_index construct_resource(Ts&&... args);
    template<typename T, typename Mat, typename... Ts> requires std::is_same_v<T, asset::material_asset>
    res_index construct_resource(Ts&&... args);

    template<typename T>
    T& get_shader_ubo(res_index shader, u32_t binding);

    struct {
        transform trans{ .position{0,0,0}, .scale{ 1, 1, 1}, .rotation{ 0, 0, 0, 1 } }; // scale ignored
        f32_t fov = 90;
        f32_t aspect; // set to w/h ration on screen change
    } _camera;

    // NOTE : edit this one in children - get fucked
    f64_t _elapsed_time;
    f64_t _delta_time;

    // input
    struct {
        f32_t x = 0;
        f32_t y = 0;
    } _keyboard_axis;

    struct {
        f32_t dx = 0;
        f32_t dy = 0;
    } _mouse_axis;

    f32_t _wheel_delta = 0;

private:
    void update_camera();

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void wheel_callback(GLFWwindow* window, double x, double y);
    static void mouse_callback(GLFWwindow* window, double x, double y);

    wsi::window _window;
    vulkan_renderer _renderer;
    asset::asset_registry _asset_reg;
    asset::asset_resource_adapter _resource_adapter;

    decltype(std::chrono::steady_clock::now()) _t0;

    f32_t _mouse_prev_y = 0;
    f32_t _mouse_prev_x = 0;
};



template<typename T, typename... Ts>
dry_program::asset_index dry_program::create_asset(Ts&&... args) {
    return _asset_reg.create<T>(std::forward<Ts>(args)...).hash;
}

template<typename T>
T& dry_program::get_asset(asset_index index) {
    return _asset_reg.get<T>(index);
}

template<typename T>
T& dry_program::get_asset(const std::string& name) {
    return _asset_reg.get<T>(name);
}

template<typename T> requires (!std::is_same_v<T, asset::material_asset>)
dry_program::res_index dry_program::create_resource(asset_index index) {
    return _resource_adapter.get_resource_index<T>(index);
}

template<typename T, typename Mat> requires std::is_same_v<T, asset::material_asset>
dry_program::res_index dry_program::create_resource(asset_index index) {
    return _resource_adapter.get_resource_index<T, Mat>(index);
}

template<typename T> requires (!std::is_same_v<T, asset::material_asset>)
dry_program::res_index dry_program::create_resource(const std::string& name) {
    return _resource_adapter.get_resource_index<T>(get_asset<T>(name).hash);
}

template<typename T, typename... Ts> requires (!std::is_same_v<T, asset::material_asset>)
dry_program::res_index dry_program::construct_resource(Ts&&... args) {
    return create_resource<T>(create_asset<T>(std::forward<Ts>(args)...));
}

template<typename T, typename Mat, typename... Ts> requires std::is_same_v<T, asset::material_asset>
dry_program::res_index dry_program::construct_resource(Ts&&... args) {
    return create_resource<T, Mat>(create_asset<T>(std::forward<Ts>(args)...));
}

template<typename T>
T& dry_program::get_shader_ubo(res_index shader, u32_t binding) {
    return _renderer.get_ubo<T>(shader, binding);
}

}

#endif