#pragma once

#include <vector>

#include "../proc/shader_headers/def_vk.hpp"
#include "../proc/shader_headers/dyn_vk.hpp"
#include "../proc/shader_headers/def.hpp" // TODO : FIRST : move write vertex to this, remove the type in pcsr, you have pre builds here, you only check the pattern and set the flag

namespace gr::res
{
    struct ShaderCtx
    {
        const char* name;
        pcsr::ShaderData data;
    };
    struct MaterialCtx
    {
        const char* name;
        const char* shaderName;
        bool blendEnabled;
    };
    struct TextureCtx
    {
        const char* name;
        const char* path;
    };

    // TODO : tmp don't know how to write vertex data yet
    void writeDefVertex(std::span<const glm::vec3> pos, std::span<const glm::vec2> uv, std::vector<pcsr::def::VertexInput>& outVert, std::vector<uint32_t>& outInd);

#define MODEL_NAME(name) "../../res/models/" #name
#define TEXTURE_NAME(name) "../../res/textures/" #name

    static constexpr std::array<const char*, 5> _OBJ_NAMES{ "building", "granite", "viking_room", "volga", "device" };

    constexpr const char* _SHADER_DIR = "../../proc/shader_bin/";
    constexpr std::array<ShaderCtx, 2> _SHADERS{ ShaderCtx{ "def", pcsr::def::defData }, ShaderCtx{ "dyn", pcsr::dyn::dynData } };
    constexpr std::array<MaterialCtx, 2> _MATERIALS{ MaterialCtx{ "defMat", "def", true }, MaterialCtx{ "dynMat", "dyn", true } };
    constexpr std::array<const char*, 5> _MODELS{
        MODEL_NAME(building.obj),
        MODEL_NAME(granite.obj),
        MODEL_NAME(viking_room.obj),
        MODEL_NAME(volga.obj),
        MODEL_NAME(device.obj)
    };
    constexpr std::array<TextureCtx, 5> _TEXTURES{
        TextureCtx{ "building", TEXTURE_NAME(building.png) },
        TextureCtx{ "granite", TEXTURE_NAME(granite.jpg) },
        TextureCtx{ "viking_room", TEXTURE_NAME(viking_room.png) },
        TextureCtx{ "volga", TEXTURE_NAME(volga.png) },
        TextureCtx{ "device", TEXTURE_NAME(device.png) }
    };
}