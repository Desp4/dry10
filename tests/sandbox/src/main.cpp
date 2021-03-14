#include "ecs/ecs.hpp"
#include "graphics/dryprogram.hpp"
#include "asset/assetreg.hpp"

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

// NOTE : just a dumb test, don't cry about the quality yet
class sandbox_program : public dry::gr::dry_program {
public:
    sandbox_program() :
        dry::gr::dry_program()
    {
        const std::string name = "viking_room";
        _asset_reg.add_resource_block("../../../tests/sandbox/assets/blob.dab");
        _asset_reg.load<dry::asset::mesh_asset>(name);
        _asset_reg.load<dry::asset::shader_asset>("def");
        _asset_reg.load<dry::asset::texture_asset>(name);
    }

protected:
    void on_start() override {
        dry::gr::material material;
        material.shader = &_asset_reg.get<dry::asset::shader_asset>("def");
        material.textures.resize(1);
        material.textures[0] = &_asset_reg.get<dry::asset::texture_asset>("viking_room");
        const auto& mesh = _asset_reg.get<dry::asset::mesh_asset>("viking_room");

        auto populate = [&](int iter, float y, float z) {
            for (auto i = 0; i < iter; ++i) {
                const auto entity = _ec_reg.create();
                // create ubo interface
                UBO ubo;
                ubo.proj = glm::perspective(
                    glm::radians(80.0f),
                    dry_program::DEFAULT_WIN_WIDTH / float(dry_program::DEFAULT_WIN_HEIGHT),
                    0.1f,
                    64.0f
                );
                ubo.proj[1][1] *= -1;
                ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                ubo.model = glm::scale(ubo.model, { 0.5f, 0.5f, 0.5f });
                const glm::vec3 front = {
                    cos(0) + cos(0),
                    sin(0),
                    sin(0) * cos(0) };
                const glm::vec3 up{ 0.0f, 1.0f, 0.0f };
                const glm::vec3 origin{ 0.0f, 0.0f, 0.0f };
                const glm::vec3 objPos{ 2.0f + i, y, z };
                const auto view = glm::lookAt(origin, origin + front, up);
                ubo.view = glm::translate(view, objPos);

                dry::gr::renderable renderable = _res_man.create_renderable(material, mesh);

                // attach
                _ec_reg.attach<dry::gr::renderable>(entity, std::move(renderable));
                _ec_reg.attach<UBO>(entity, std::move(ubo));
            }
        };
        
        populate(64, -0.5f, -1.0f);
        populate(64, -0.5f, 1.0f);
        populate(64, 0.5f, -1.0f);
        populate(64, 0.5f, 1.0f);
        populate(64, -1.5f, -1.0f);
        populate(64, -1.5f, 1.0f);
        populate(64, 1.5f, -1.0f);
        populate(64, 1.5f, 1.0f);
    }
    void update() override {
        static auto prev_time = std::chrono::high_resolution_clock::now();
        static auto curr_time = prev_time;

        prev_time = curr_time;
        curr_time = std::chrono::high_resolution_clock::now();
        const double dt = std::chrono::duration_cast<std::chrono::duration<double>>(curr_time - prev_time).count();
        _window.set_title(std::to_string(dt));

        auto comp_view = _ec_reg.view<dry::gr::renderable, UBO>();

        for (auto entity : comp_view) {
            auto [renderable, ubo] = comp_view.get<dry::gr::renderable, UBO>(entity);

            ubo.model = glm::rotate(ubo.model, static_cast<float>(1.0f * dt), glm::vec3(0.0f, 0.0f, 1.0f));
            _res_man.write_to_buffer(renderable, 0, &ubo, sizeof ubo);
        }
    }

private:
    dry::asset::asset_registry _asset_reg;
    dry::ecs::ec_registry _ec_reg;

    struct UBO {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
};

int main()
{
    dry::wsi::gldw_dummy glfw_dummy;
    sandbox_program program;

    program.begin_render();

    return 0;
}