#include "graphics/dryprogram.hpp"

#include <glm/gtc/matrix_transform.hpp>
// NOTE : just a dumb test, don't cry about the quality yet
class SandboxProgram : public gr::DryProgram
{
public:
    SandboxProgram() : gr::DryProgram()
    {
        const std::string name = "viking_room";
        asset::AssetRegistry::filesystem.addDirectory("../../../tests/sandbox/assets");
        _assetReg.load<asset::MeshAsset>(name.c_str());
        _assetReg.load<asset::ShaderAsset>("def");
        _assetReg.load<asset::TextureAsset>(name.c_str());

        gr::core::Material material;
        material.shader = &_assetReg.get<asset::ShaderAsset>("def");
        material.textures.resize(1);
        material.textures[0] = &_assetReg.get<asset::TextureAsset>(name);

        const auto& mesh = _assetReg.get<asset::MeshAsset>(name);

        _renderable = _resManager.createRenderable(material, mesh);

        _ubo.proj = glm::perspective(
            glm::radians(80.0f),
            DryProgram::DEFAULT_WIN_WIDTH / float(DryProgram::DEFAULT_WIN_HEIGHT),
            0.1f,
            64.0f);
        _ubo.proj[1][1] *= -1;
        _ubo.model = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    }

protected:
    void onStart() override {}
    void update() override
    {
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

        if (_flag2)
            _resManager.writeToUniformBuffer(_renderable, 0, &_ubo, sizeof _ubo);
        if (!_flag)
            _resManager.writeToUniformBuffer(_renderable2, 0, &_ubo2, sizeof _ubo2);


        _elapsed += 0.001f;
        if (_elapsed > 3.0f && _flag)
        {
            _flag = false;
            const std::string name = "viking_room";

            gr::core::Material material;
            material.shader = &_assetReg.get<asset::ShaderAsset>("def");
            material.textures.resize(1);
            material.textures[0] = &_assetReg.get<asset::TextureAsset>(name);

            const auto& mesh = _assetReg.get<asset::MeshAsset>(name);

            _renderable2 = _resManager.createRenderable(material, mesh);

            _ubo2.proj = glm::perspective(glm::radians(80.0f), 1280.0f / 720.0f, 0.1f, 64.0f);
            _ubo2.proj[1][1] *= -1;
            _ubo2.model = glm::rotate(glm::mat4(1.0f), 1.0f, glm::vec3(1.0f, 0.0f, 0.0f));

            _ubo2.view = glm::translate(view, { 2, 0, 0 });

            _resManager.writeToUniformBuffer(_renderable2, 0, &_ubo2, sizeof _ubo2);
        }

        if (_elapsed > 8.0f && _flag2)
        {
            _flag2 = false;
            _resManager.destroyRenderable(_renderable);
        }
    }

private:
    gr::core::RenderableHandle _renderable;
    gr::core::RenderableHandle _renderable2;
    struct UBO
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
    UBO _ubo;
    UBO _ubo2;
    float _elapsed = 0.0f;
    bool _flag = true;
    bool _flag2 = true;
};

int main()
{
    wsi::GLFWDummy glfw;
    SandboxProgram program;

    program.beginRender();

    return 0;
}