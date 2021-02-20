#include "graphics/dryprogram.hpp"

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

        dry::gr::material material;
        material.shader = &_asset_reg.get<dry::asset::shader_asset>("def");
        material.textures.resize(1);
        material.textures[0] = &_asset_reg.get<dry::asset::texture_asset>(name);

        const auto& mesh = _asset_reg.get<dry::asset::mesh_asset>(name);

        _renderable = _res_man.create_renderable(material, mesh);

        _ubo.proj = glm::perspective(
            glm::radians(80.0f),
            dry_program::DEFAULT_WIN_WIDTH / float(dry_program::DEFAULT_WIN_HEIGHT),
            0.1f,
            64.0f
        );
        _ubo.proj[1][1] *= -1;
        _ubo.model = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    }

protected:
    void on_start() override {}
    void update() override {
        const glm::vec3 front = {
            cos(0) + cos(0),
            sin(0),
            sin(0) * cos(0) };
        const glm::vec3 up{ 0.0f, 1.0f, 0.0f };
        const glm::vec3 origin{ 0.0f, 0.0f, 0.0f };
        const glm::vec3 objPos{ 2.0f, -0.5f, 0.0f };

        const auto view = glm::lookAt(origin, origin + front, up);

        _ubo.view = glm::translate(view, objPos);
        _ubo.model = glm::rotate(_ubo.model, 0.0005f, glm::vec3(0.0f, 1.0f, 0.0f));
        _res_man.write_to_buffer(_renderable, 0, &_ubo, sizeof _ubo);
    }

private:
    dry::gr::renderable _renderable;
    struct UBO {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
    UBO _ubo;
};

int main()
{
    dry::wsi::gldw_dummy glfw_dummy;
    sandbox_program program;

    program.begin_render();

    return 0;
}