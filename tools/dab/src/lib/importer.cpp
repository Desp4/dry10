#include "dab/import.hpp"

#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "share/dab_type.hpp"
#include "share/shpb_type.hpp"

namespace dab
{
    bool BlockImporter::open(const std::filesystem::path& path)
    {
        _file.open(path, std::ios_base::binary);
        if (!_file.is_open())
        {
            _file.clear();
            return false;
        }

        char magicBuffer[sizeof dab::FILE_MAGIC]{ 0 };
        _file.read(magicBuffer, sizeof magicBuffer);
        if (memcmp(magicBuffer, dab::FILE_MAGIC, sizeof dab::FILE_MAGIC))
        {
            _file.close();
            return false;
        }

        return true;
    }

    void BlockImporter::close()
    {
        _file.close();
    }

    template<>
    Mesh BlockImporter::read<Mesh>(size_t offset)
    {
        _file.seekg(offset, std::ios_base::beg);

        uint32_t vertCount = 0;
        uint8_t uvSetCount = 0;
        _file.read(reinterpret_cast<char*>(&vertCount), sizeof vertCount);
        _file.read(reinterpret_cast<char*>(&uvSetCount), sizeof uvSetCount);

        Mesh mesh;
        mesh.vertexData.resize(vertCount * sizeof(float) * 3);
        mesh.texCoordSets.resize(uvSetCount);
        
        _file.read(reinterpret_cast<char*>(mesh.vertexData.data()), mesh.vertexData.size());
        for (auto& set : mesh.texCoordSets)
        {
            _file.read(reinterpret_cast<char*>(&set.componentCount), sizeof set.componentCount);
            set.texCoordData.resize(sizeof(float) * set.componentCount * vertCount);

            _file.read(reinterpret_cast<char*>(set.texCoordData.data()), set.texCoordData.size());
        }

        return mesh;
    }

    template<>
    Texture BlockImporter::read<Texture>(size_t offset)
    {
        _file.seekg(offset, std::ios_base::beg);

        size_t size = 0;
        _file.read(reinterpret_cast<char*>(&size), sizeof size);

        std::vector<uint8_t> imgData(size);
        _file.read(reinterpret_cast<char*>(imgData.data()), size);

        int w = 0, h = 0, ch = 0;
        // NOTE : 8bit, prefer rgba
        stbi_uc* data = stbi_load_from_memory(imgData.data(), size, &w, &h, &ch, STBI_rgb_alpha);

        Texture texture{
            .channels = 4, // NOTE : forcing alpha
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h) };
        texture.pixelData.assign(data, data + w * h * texture.channels);

        stbi_image_free(data);
        return texture;
    }

    template<>
    Shader BlockImporter::read<Shader>(size_t offset)
    {
        // skip reading file magic and file size
        _file.seekg(offset + sizeof shpb::FILE_MAGIC + sizeof(size_t), std::ios_base::beg);

        uint32_t stageCount = 0;
        _file.read(reinterpret_cast<char*>(&stageCount), sizeof stageCount);

        Shader shader;
        shader.modules.resize(stageCount);

        for (auto& stageModule : shader.modules)
        {
            char buffer[sizeof(shpb::ShaderType) + sizeof(size_t)]{ 0 };
            _file.read(buffer, sizeof buffer);

            stageModule.stage = static_cast<VkShaderStageFlagBits>(
                *reinterpret_cast<shpb::ShaderType*>(buffer));

            stageModule.data.resize(
                *reinterpret_cast<size_t*>(buffer + sizeof shpb::ShaderType) / sizeof(uint32_t));

            _file.read(reinterpret_cast<char*>(stageModule.data.data()), stageModule.data.size() * sizeof(uint32_t));
        }

        return shader;
    }

    std::vector<AssetDecl> BlockImporter::assetDeclarations()
    {
        _file.seekg(sizeof dab::FILE_MAGIC, std::ios_base::beg);

        size_t declOffset = 0;
        _file.read(reinterpret_cast<char*>(&declOffset), sizeof declOffset);
        _file.seekg(declOffset, std::ios_base::beg);

        uint32_t declCount = 0;
        _file.read(reinterpret_cast<char*>(&declCount), sizeof declCount);

        std::vector<AssetDecl> declarations(declCount);

        for (auto& declaration : declarations)
        {
            constexpr uint32_t nonStrDataSize = sizeof AssetDecl::offset + sizeof AssetDecl::type + sizeof(uint8_t);

            char buffer[0xFF]{ 0 };
            _file.read(buffer, nonStrDataSize);

            declaration.offset = *reinterpret_cast<decltype(AssetDecl::offset)*>(buffer);
            declaration.type   = *reinterpret_cast<decltype(AssetDecl::type)*>(buffer + sizeof(AssetDecl::offset));

            const uint8_t stringLen = *reinterpret_cast<uint8_t*>(
                buffer + sizeof(AssetDecl::offset) + sizeof(AssetDecl::type));

            _file.read(buffer, stringLen);
            declaration.name.assign(buffer, buffer + stringLen);
        }

        return declarations;
    }
}