#pragma once

#include <vector>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// NOTE : not a fan of this
#include "../core/renderer.hpp"
#include "../../vkw/desc/descpool.hpp"
#include "../proc/shader_headers/def.hpp" // tmp
// NOTE : not a fan of sharing device and queue and framecount for that matter, move it to a separate thing not in a renderer perhaps
namespace gr
{
    class ResourceDB
    {
    public:
        // Data is loaded from a header in a cpp file
        ResourceDB(const vkw::Device* device, const vkw::GraphicsQueue* grQueue, const vkw::TransferQueue* trQueue);

        vkw::Renderable createRenderable(uint32_t matID, const char* modName, std::span<const char* const> texNames) const;
        vkw::Material vkMaterial(const char* matName) const;
        uint32_t materialID(const char* matName) const;

    private:
        struct Shader
        {
            std::string name;
            pcsr::ShaderData data;
            vkw::ShaderModule modules[2];
            vkw::DescriptorLayout layout;
        };

        struct Texture
        {
            std::string name;
            int32_t width;
            int32_t height;
            int32_t channels;
            std::vector<uint8_t> data;
        };

        struct Model
        {
            std::string name;
            std::vector<glm::vec3> pos;
            std::vector<glm::vec2> uv;
        };

        struct Material
        {
            std::string name;
            uint32_t shaderID;
            bool blendEnabled;
        };

        static std::vector<uint32_t> dumpShaderBin(const std::string& path);

        vkw::DevicePtr _device;
        util::NullablePtr<const vkw::GraphicsQueue> _graphicsQueue;
        util::NullablePtr<const vkw::TransferQueue> _transferQueue;
        vkw::DescriptorPool _descPool;

        std::vector<Shader> _shaders;
        std::vector<Texture> _textures;
        std::vector<Model> _models;
        std::vector<Material> _materials;

        // TODO : figure out what to do with this
        std::vector<std::vector<pcsr::def::VertexInput>> _defVertInput;
        std::vector<std::vector<uint32_t>> _defVertInds;

        static constexpr uint32_t _DESC_CAPACITY_PER_FRAME = 64; // TODO : those should be dynamic to some degree
        static constexpr uint32_t _DESC_POOL_CAPACITY = 512;
    };
}