#pragma once

#include <vector>
#include <fstream>

#include <vulkan/vulkan.h>

#include "asset.hpp"

namespace dab
{
    struct Mesh
    {
        static constexpr AssetType type = Asset_Mesh;

        struct TexCoordSet
        {
            std::vector<uint8_t> texCoordData;
            uint8_t componentCount;
        };

        std::vector<uint8_t> vertexData;
        std::vector<TexCoordSet> texCoordSets;
    };

    struct Texture
    {
        static constexpr AssetType type = Asset_Texture;

        uint8_t channels;
        uint32_t width;
        uint32_t height;
        std::vector<uint8_t> pixelData;
    };

    struct ShaderVkData
    {
        template<typename T>
        struct DescriptorInfo
        {
            uint32_t layoutBindingInd;
            T info;
        };

        VkVertexInputBindingDescription vertexBinding;
        std::vector<VkVertexInputAttributeDescription> vertexDescriptors;

        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

        std::vector<DescriptorInfo<VkDescriptorBufferInfo>> bufferInfos;
        std::vector<DescriptorInfo<VkDescriptorImageInfo>> combImageInfos;
    };

    struct Shader
    {
        static constexpr AssetType type = Asset_Shader;

        struct ShaderModule
        {
            VkShaderStageFlagBits stage;
            std::vector<uint32_t> data;
        };

        std::vector<ShaderModule> modules;

        ShaderVkData vkData() const;
    };

    class BlockImporter
    {
    public:
        bool open(const std::filesystem::path& path);
        void close();

        std::vector<AssetDecl> assetDeclarations();

        template<class T>
        T read(size_t offset);

    private:
        std::ifstream _file;
    };

    template<>
    Mesh BlockImporter::read(size_t);
    template<>
    Texture BlockImporter::read(size_t);
    template<>
    Shader BlockImporter::read(size_t);

    template<class T>
    T BlockImporter::read(size_t offset)
    {
        static_assert(false, "unspecialized use is forbindden");
    }
}