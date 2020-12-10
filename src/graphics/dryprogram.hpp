#pragma once

#include "core/renderer.hpp"
#include "db/resourcedb.hpp"

#include "../proc/shader_headers/def.hpp"

namespace gr
{
    struct PipelineRenderables
    {
        uint32_t matID;
        std::vector<vkw::Renderable> renderables;
    };

    struct SceneObject
    {
        vkw::Renderable* renderable;
        pcsr::def::UniformBufferObject transform;
        glm::vec3 pos;
    };

    struct UserData // TODO : this can be one abstraction above renderable with ubo pos and renderable
    {
        std::vector<SceneObject> objects;

        glm::vec3 pos{ 0.0f };

        uint32_t objIt = 0;

        static constexpr float SPACE_MAX_COOLDOWN = 0.8f;
        float spaceCooldown = 0.0f;

        bool spin = false;
    };

    struct WindowState
    {
        float yaw = -90.0f;
        float pitch = 0.0f;

        float lastMouseX = 0.0f;
        float lastMouseY = 0.0f;

        double lastTime;
    };

    // TODO : singleton or whatever, db and renderer too perhaps
    class DryProgram
    {
    public:
        DryProgram();

        void beginRender();

    protected:
        static void onDraw(uint32_t frame, void* data);

        void onStart();
        glm::vec2 getKeyInput();

    private:
        static void mouseCallback(GLFWwindow* window, double x, double y);

        wsi::Window _window;
        core::Renderer _renderer;
        ResourceDB _resDB;

        std::vector<PipelineRenderables> _renderables;

        UserData _userData;
        WindowState _windowState;

        static constexpr uint32_t _WIN_WIDTH  = 1280;
        static constexpr uint32_t _WIN_HEIGHT = 720;
    };
}