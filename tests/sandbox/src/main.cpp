#include <glm/gtc/matrix_transform.hpp>

#include "graphics/renderer.hpp"
#include "asset/asset_resource_adapter.hpp"

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

constexpr u32_t circle_count = 40;
constexpr u32_t circle_obj_count = 200;
constexpr f32_t angle_offset = 2 * glm::pi<f32_t>() / circle_obj_count;

constexpr u32_t delete_rend_start_us = 100000;
constexpr u32_t new_rend_start_us = 0;
constexpr u32_t mutate_circle_us = 200000;

constexpr std::array<std::string_view, 3> shader_names{ "def_tex_inst", "def_inv_inst", "def_pos_inst" };
constexpr std::array<std::string_view, 3> mesh_names{ "volga", "viking_room", "granite" };
constexpr std::array mesh_scales{ 10.0f / 10.0f, 0.2f / 10.0f, 10.0f / 10.0f };

constexpr u32_t win_width = 800;
constexpr u32_t win_height = 800;

constexpr f32_t camera_speed = 75.0f;
constexpr f32_t camera_sensetivity = 0.01f;
constexpr f32_t scroll_speed = 0.05f;

struct rend_pair {
    vulkan_renderer::renderable_id handle;
    object_transform transform;
    glm::mat4 pos;
    glm::mat4 scale;
};
struct object_circle {
    f32_t distance;
    f32_t anglular_offset;
    f32_t angular_speed;
    f32_t direction;
    f32_t radius;
    std::vector<rend_pair> rends;
};


i32_t input_x = 0;
i32_t input_y = 0;

f32_t mouse_x = 0.0f;
f32_t mouse_y = 0.0f;

f32_t scroll_y = 0.0f;

void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    i32_t delta = 0;
    i32_t* target;

    switch (key) {
    case GLFW_KEY_W: target = &input_x; delta = 1; break;
    case GLFW_KEY_S: target = &input_x; delta = -1; break;
    case GLFW_KEY_D: target = &input_y; delta = 1; break;
    case GLFW_KEY_A: target = &input_y; delta = -1; break;
    default: return;
    }

    if (action == GLFW_RELEASE) {
        *target = 0;
    }
    else {
        *target = delta;
    }
}

void glfw_scroll_callback(GLFWwindow* window, double x, double y) {
    scroll_y = static_cast<f32_t>(y);
}

void glfw_mouse_callback(GLFWwindow* window, double x, double y) {
    static double prev_x = 0.0;
    static double prev_y = 0.0;

    mouse_x = static_cast<f32_t>(x - prev_x);
    mouse_y = static_cast<f32_t>(-(y - prev_y)); // NOTE : inverted

    prev_x = x;
    prev_y = y;
}

auto create_meshes(asset::asset_registry& reg, asset::asset_resource_adapter& adapter) {
    std::array<asset::asset_resource_adapter::index_type, mesh_names.size()> meshes;
    for (auto i = 0u; i < meshes.size(); ++i) {
        const auto mesh_asset_h = reg.get<asset::mesh_asset>(mesh_names[i].data()).hash;
        meshes[i] = adapter.get_resource_index<asset::mesh_asset>(mesh_asset_h);
    }
    return meshes;
}

auto create_materials(asset::asset_registry& reg, asset::asset_resource_adapter& adapter) {
    std::array<std::array<asset::asset_resource_adapter::index_type, mesh_names.size()>, shader_names.size()> materials;
    for (auto i = 0u; i < materials.size(); ++i) {
        const auto def_h = reg.get<asset::shader_asset>(shader_names[i].data()).hash;

        if (shader_names[i] == "def_pos_inst") {
            empty_material* material_src = new empty_material{};
            const auto material = reg.create<asset::material_asset>(def_h, material_src).hash;
            const auto material_h = adapter.get_resource_index<asset::material_asset, empty_material>(material);

            for (auto& elem : materials[i]) {
                elem = material_h;
            }
        }
        else {
            for (auto j = 0u; j < materials[i].size(); ++j) {
                const auto tex_asset_h = adapter.get_resource_index<asset::texture_asset>(reg.get<asset::texture_asset>(mesh_names[j].data()).hash);

                textured_material* material_src = new textured_material{ tex_asset_h };
                const auto material = reg.create<asset::material_asset>(def_h, material_src).hash;
                materials[i][j] = adapter.get_resource_index<asset::material_asset, textured_material>(material);
            }
        }
    }
    return materials;
}

glm::mat4 scale_matrix(float val) {
    return glm::scale(glm::mat4{ 1.0 }, glm::vec3(val, val, val));
};

void consume_mouse_delta() {
    mouse_x = 0.0f;
    mouse_y = 0.0f;
}

void consume_scroll_delta() {
    scroll_y = 0.0f;
}

int main() {
    // init
    wsi::window window{ win_width, win_height };
    vulkan_renderer vk_rend{ window };
    asset::asset_registry asset_reg;
    asset::asset_resource_adapter res_adapter{ asset_reg };
    res_adapter.attach_renderer(vk_rend);

    camera_transform camera;

    vk_rend.bind_camera_transform(camera);

    constexpr glm::vec3 camera_up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 camera_pos{ 0.0f, 0.0f, 0.0f };
    glm::vec3 camera_front{ 0.0f, 0.0f, 1.0f };
    f32_t pitch = 0.0f;
    f32_t yaw = 0.0f;
    f32_t fov = 80.0f;

    glfwSetInputMode(window.handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window.handle(), glfw_key_callback);
    glfwSetCursorPosCallback(window.handle(), glfw_mouse_callback);
    glfwSetScrollCallback(window.handle(), glfw_scroll_callback);

    const auto meshes = create_meshes(asset_reg, res_adapter);
    const auto materials = create_materials(asset_reg, res_adapter);

    // init groups
    std::vector<object_circle> obj_groups(circle_count);
    for (auto i = 0u; i < obj_groups.size(); ++i) {
        obj_groups[i].rends.resize(circle_obj_count);

        obj_groups[i].distance = -150.0f - 25.0f * i;
        obj_groups[i].anglular_offset = 0.76f * i;
        obj_groups[i].angular_speed = 0.0000002f + i * 0.00000005f;
        obj_groups[i].direction = (i % 2) ? -1.0f : 1.0f;
        obj_groups[i].radius = 100.0f - 2.0f * i;
    }

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    // spawn all
    for (auto& group : obj_groups) {
        for (auto& rend : group.rends) {
            u32_t mat_ind = rand() % 3;
            u32_t mesh_ind = rand() % 3;

            rend.pos = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, group.distance));
            rend.scale = scale_matrix(mesh_scales[mesh_ind]);
            rend.transform.model = glm::rotate(rend.pos, glm::radians(-90.0f), { 0.0f, 1.0f, 0.0f });
            rend.transform.model = rend.transform.model * rend.scale;

            rend.handle = vk_rend.create_renderable(materials[mat_ind][mesh_ind], meshes[mesh_ind]);
            vk_rend.bind_renderable_transform(rend.handle, rend.transform);
        }
    }

    u64_t t_elapsed = 0;
    u64_t t_delete = delete_rend_start_us;
    u64_t t_new = new_rend_start_us;

    u32_t i_delete = 0;
    u32_t i_new = 0;

    // framerate array
    u32_t curr_frame = 0;
    std::vector<u64_t> frame_elapsed_us(512, 0);

    auto t0 = std::chrono::steady_clock::now();
    while (!window.should_close()) {
        window.poll_events();

        // time things
        const auto t1 = std::chrono::steady_clock::now();
        const auto dt = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        t0 = t1;

        t_elapsed += dt;
        t_delete += dt;
        t_new += dt;

        frame_elapsed_us[curr_frame] = dt;
        curr_frame = (curr_frame + 1) % frame_elapsed_us.size();

        const f32_t dt_s = static_cast<f32_t>(dt) / 1000000.0f;

        // logic
        fov += scroll_y * scroll_speed;
        fov = std::clamp(fov, glm::radians(5.0f), glm::radians(120.0f));
        consume_scroll_delta();

        yaw += mouse_x * camera_sensetivity;
        pitch += mouse_y * camera_sensetivity;
        pitch = std::clamp(pitch, glm::radians(-89.0f), glm::radians(89.0f));
        consume_mouse_delta();

        camera_pos += input_x * camera_speed * dt_s * camera_front;
        camera_pos += input_y * camera_speed * dt_s * glm::normalize(glm::cross(camera_front, camera_up));

        camera_front.x = std::cosf(yaw) * std::cosf(pitch);
        camera_front.y = std::sinf(pitch);
        camera_front.z = std::sinf(yaw) * std::cosf(pitch);
        camera_front = glm::normalize(camera_front);

        camera.proj = glm::perspective(fov, static_cast<f32_t>(win_width / win_height), 0.1f, 8000.0f);
        camera.proj[1][1] *= -1;
        camera.view = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);
        camera.viewproj = camera.proj * camera.view;

        if (t_delete >= mutate_circle_us) {
            for (auto& group : obj_groups) {
                vk_rend.destroy_renderable(group.rends[i_delete].handle);
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
                rend.handle = vk_rend.create_renderable(materials[mat_ind][mesh_ind], meshes[mesh_ind]);
                vk_rend.bind_renderable_transform(rend.handle, rend.transform);
            }

            i_new = (i_new + 1) % circle_obj_count;
            t_new = 0;
        }

        for (auto& group : obj_groups) {
            for (auto i = 0u; i < group.rends.size(); ++i) {
                auto& rend = group.rends[i];
                const f32_t angle = group.direction * (t_elapsed * group.angular_speed + angle_offset * i + group.anglular_offset);

                rend.transform.model = glm::translate(rend.pos,
                    glm::vec3(group.radius * std::cosf(angle), group.radius * std::sinf(angle), 0.0f)
                );
                rend.transform.model = glm::rotate(rend.transform.model, glm::radians(-90.0f), { 0.0f, 1.0f, 0.0f });
                rend.transform.model = rend.transform.model * rend.scale;
            }
        }

        vk_rend.submit_frame();
    }

    float total = 0;
    for (auto dt : frame_elapsed_us) {
        total += static_cast<float>(dt);
    }
    total = total / (frame_elapsed_us.size() * 1000.0f);

    printf("average frame time: %f ms, fps %i\n", total, static_cast<u32_t>(1000.0f / total));

    return 0;
}