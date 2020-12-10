#include "dryprogram.hpp"

#include "db/resources.hpp"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace gr
{
    void DryProgram::mouseCallback(GLFWwindow* window, double x, double y)
    {
        auto& input = static_cast<DryProgram*>(glfwGetWindowUserPointer(window))->_windowState;
        const float dx = (x - input.lastMouseX) * 0.005f;
        const float dy = (y - input.lastMouseY) * 0.005f;
        input.lastMouseX = x;
        input.lastMouseY = y;

        input.yaw += dx;
        input.pitch -= dy;
        if (input.pitch > glm::radians(89.0f))
            input.pitch = glm::radians(89.0f);
        if (input.pitch < -glm::radians(89.0f))
            input.pitch = -glm::radians(89.0f);
    }

    void DryProgram::onDraw(uint32_t frame, void* data)
    {
        DryProgram& inst = *static_cast<DryProgram*>(data);
        UserData& uData = inst._userData;
        const float dt = glfwGetTime() - inst._windowState.lastTime;
        inst._windowState.lastTime += dt;

        glm::vec3 front = {
            cos(inst._windowState.yaw) * cos(inst._windowState.pitch),
            sin(inst._windowState.pitch),
            sin(inst._windowState.yaw) * cos(inst._windowState.pitch) };
        front = glm::normalize(front);

        const glm::vec2 input = inst.getKeyInput();
        const glm::vec3 up{ 0.0f, 1.0f, 0.0f };
        uData.pos += dt * 16.0f * (front * input.y + glm::normalize(glm::cross(front, up)) * input.x);
        const auto view = glm::lookAt(uData.pos, uData.pos + front, up);

        uData.spaceCooldown -= dt;
        if (uData.spaceCooldown < 0.0f)
        {
            if (glfwGetKey(inst._window.window(), GLFW_KEY_SPACE) == GLFW_PRESS)
            {
                uData.spaceCooldown = uData.SPACE_MAX_COOLDOWN;
                uData.spin = !uData.spin;
            }

            /*
            if (glfwGetKey(inst._window.window(), GLFW_KEY_SPACE) == GLFW_PRESS)
            {
                uData.spaceCooldown = uData.SPACE_MAX_COOLDOWN;

                SceneObject obj;
                const char* texName = res::_TEXTURES[uData.objIt % res::_TEXTURES.size()].name;
                inst._renderables[uData.objIt % inst._renderables.size()].renderables.push_back(
                    inst._resDB.createRenderable(uData.objIt % inst._renderables.size(), res::_OBJ_NAMES[uData.objIt % res::_OBJ_NAMES.size()], std::span{ &texName, 1 }));

                obj.renderable = &inst._renderables[uData.objIt % inst._renderables.size()].renderables.back();
                obj.pos = uData.pos + 8.0f * front;
                obj.transform.model = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
                obj.transform.proj = glm::perspective(glm::radians(80.0f), _WIN_WIDTH / float(_WIN_HEIGHT), 0.1f, 64.0f);
                obj.transform.proj[1][1] *= -1;
                uData.objects.push_back(std::move(obj));
                ++uData.objIt;
            }*/
        }
        /*
        for (auto& obj : uData.objects)
        {
            obj.transform.view = glm::translate(view, obj.pos);

            obj.renderable->frameData[frame].ubos[0].ubo.writeToMemory(&obj.transform, sizeof obj.transform);
        }*/

        uData.objects[0].transform.view = glm::translate(view, uData.objects[0].pos);
        uData.objects[1].transform.view = glm::translate(view, uData.objects[1].pos);

        if (uData.spin)
        {
            uData.objects[0].transform.model = glm::rotate(uData.objects[0].transform.model, dt, { 0.0f, 0.0f, 1.0f });

            uData.objects[1].transform.view = glm::translate(uData.objects[1].transform.view, { 0.0f, sin(inst._windowState.lastTime * 2.0f) * 4.0f, 0.0f });
        }

        uData.objects[0].renderable->frameData[frame].ubos[0].ubo.writeToMemory(&uData.objects[0].transform, sizeof uData.objects[0].transform);
        uData.objects[1].renderable->frameData[frame].ubos[0].ubo.writeToMemory(&uData.objects[1].transform, sizeof uData.objects[1].transform);
    }

    DryProgram::DryProgram() :
        _window(_WIN_WIDTH, _WIN_HEIGHT),
        _renderer(_window.windowHandle()),
        _resDB(&_renderer.device(), &_renderer.grQueue(), &_renderer.trQueue())
    {
        //glfwSetInputMode(_window.window(), GLFW_STICKY_KEYS, GL_TRUE);
        glfwSetInputMode(_window.window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetWindowUserPointer(_window.window(), this);
        glfwSetCursorPosCallback(_window.window(), mouseCallback);

        _renderer.setCallback(onDraw);
        _renderer.setUserData(this);
        // create needed pipelines ahead NOTE : might be dynamic as well although probably not
        _renderables.reserve(res::_MATERIALS.size());
        for (const auto& mat : res::_MATERIALS)
        {
            const auto vkMat = _resDB.vkMaterial(mat.name);
            _renderables.push_back(PipelineRenderables{ .matID = vkMat.matID });
            _renderer.createPipeline(vkMat, &_renderables.back().renderables);
        }
        onStart();
    }

    void DryProgram::beginRender()
    {
        _windowState.lastTime = glfwGetTime();
        while (!_window.shouldClose())
        {
            _window.pollEvents();
            _renderer.drawFrame();
        }
        _renderer.waitOnDevice();
    }

    glm::vec2 DryProgram::getKeyInput()
    {
        glm::vec2 ret{ 0.0f };
        if (glfwGetKey(_window.window(), GLFW_KEY_W) == GLFW_PRESS)
            ret.y += 1.0f;
        if (glfwGetKey(_window.window(), GLFW_KEY_S) == GLFW_PRESS)
            ret.y -= 1.0f;
        if (glfwGetKey(_window.window(), GLFW_KEY_D) == GLFW_PRESS)
            ret.x += 1.0f;
        if (glfwGetKey(_window.window(), GLFW_KEY_A) == GLFW_PRESS)
            ret.x -= 1.0f;
        // NOTE : see if epsilons cause problems
        return (ret.x == 0.0f && ret.y == 0.0f) ? ret : glm::normalize(ret);
    }

    void DryProgram::onStart()
    {
        _userData.objects.reserve(64);

        const uint32_t defID = _resDB.materialID("defMat");
        auto matArr = std::find_if(_renderables.begin(), _renderables.end(), [defID](const auto& a){ return a.matID == defID; });
        assert(matArr != _renderables.end());
        // NOTE : FIRST : pointers get invalidated
        matArr->renderables.reserve(64);

        SceneObject obj;
        const char* texName = "volga";
        matArr->renderables.push_back(_resDB.createRenderable(defID, "volga", { &texName, 1 }));

        obj.renderable = &matArr->renderables.back();
        obj.pos = { 2.0f, 0.0f, 5.0f };
        obj.transform.model = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        obj.transform.proj = glm::perspective(glm::radians(80.0f), _WIN_WIDTH / float(_WIN_HEIGHT), 0.1f, 64.0f);
        obj.transform.proj[1][1] *= -1;
        _userData.objects.push_back(std::move(obj));

        const uint32_t dynId = _resDB.materialID("dynMat");
        matArr = std::find_if(_renderables.begin(), _renderables.end(), [dynId](const auto& a) { return a.matID == dynId; });
        assert(matArr != _renderables.end());
        matArr->renderables.reserve(64);

        SceneObject obj2;
        texName = "device";
        matArr->renderables.push_back(_resDB.createRenderable(dynId, "device", { &texName, 1 }));

        obj2.renderable = &matArr->renderables.back();
        obj2.pos = { -0.0f, 2.0f, -3.0f };
        obj2.transform.model = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        obj2.transform.proj = glm::perspective(glm::radians(80.0f), _WIN_WIDTH / float(_WIN_HEIGHT), 0.1f, 64.0f);
        obj2.transform.proj[1][1] *= -1;
        _userData.objects.push_back(std::move(obj2));
    }
}