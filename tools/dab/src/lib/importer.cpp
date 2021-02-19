#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "dab/import.hpp"
#include "share/dab_type.hpp"
#include "share/shpb_type.hpp"

namespace dab {

bool dab_importer::open(const std::filesystem::path& filepath) {
    _file.open(filepath, std::ios_base::binary);
    if (!_file.is_open()) {
        _file.clear();
        return false;
    }

    char magic_buffer[sizeof dab::FILE_MAGIC]{ 0 };
    _file.read(magic_buffer, sizeof magic_buffer);
    if (memcmp(magic_buffer, dab::FILE_MAGIC, sizeof dab::FILE_MAGIC)) {
        _file.close();
        return false;
    }
    return true;
}

void dab_importer::close() {
    _file.close();
}

template<>
mesh dab_importer::read(size_t offset) {
    _file.seekg(offset, std::ios_base::beg);

    uint32_t vert_count = 0;
    uint8_t uv_set_count = 0;
    _file.read(reinterpret_cast<char*>(&vert_count), sizeof vert_count);
    _file.read(reinterpret_cast<char*>(&uv_set_count), sizeof uv_set_count);

    mesh ret_mesh;
    ret_mesh.vertex_data.resize(vert_count * sizeof(float) * 3);
    ret_mesh.tex_coord_sets.resize(uv_set_count);

    _file.read(reinterpret_cast<char*>(ret_mesh.vertex_data.data()), ret_mesh.vertex_data.size());
    for (auto& set : ret_mesh.tex_coord_sets) {
        _file.read(reinterpret_cast<char*>(&set.component_count), sizeof set.component_count);
        set.tex_data.resize(sizeof(float) * set.component_count * vert_count);

        _file.read(reinterpret_cast<char*>(set.tex_data.data()), set.tex_data.size());
    }

    return ret_mesh;
}

template<>
texture dab_importer::read(size_t offset) {
    _file.seekg(offset, std::ios_base::beg);

    size_t size = 0;
    _file.read(reinterpret_cast<char*>(&size), sizeof size);

    std::vector<uint8_t> img_data(size);
    _file.read(reinterpret_cast<char*>(img_data.data()), img_data.size());

    int w = 0, h = 0, ch = 0;
    // NOTE : 8bit, prefer rgba
    stbi_uc* data = stbi_load_from_memory(img_data.data(), size, &w, &h, &ch, STBI_rgb_alpha);

    texture ret_texture{
        .channels = 4, // NOTE : forcing alpha
        .width = static_cast<uint32_t>(w),
        .height = static_cast<uint32_t>(h)
    };
    ret_texture.pixel_data.assign(data, data + w * h * ret_texture.channels);

    stbi_image_free(data);
    return ret_texture;
}

template<>
shader dab_importer::read(size_t offset) {
    // skip reading file magic and file size
    _file.seekg(offset + sizeof shpb::FILE_MAGIC + sizeof(size_t), std::ios_base::beg);

    uint32_t stage_count = 0;
    _file.read(reinterpret_cast<char*>(&stage_count), sizeof stage_count);

    shader ret_shader;
    ret_shader.modules.resize(stage_count);

    for (auto& stage_module : ret_shader.modules) {
        char buffer[sizeof(shpb::shader_type) + sizeof(size_t)]{ 0 };
        _file.read(buffer, sizeof buffer);

        stage_module.stage = static_cast<VkShaderStageFlagBits>(*reinterpret_cast<shpb::shader_type*>(buffer));

        stage_module.module_data.resize(*reinterpret_cast<size_t*>(buffer + sizeof shpb::shader_type) / sizeof(uint32_t));
        _file.read(reinterpret_cast<char*>(stage_module.module_data.data()), stage_module.module_data.size() * sizeof(uint32_t));
    }
    return ret_shader;
}

std::vector<asset_decl> dab_importer::declarations() {
    _file.seekg(sizeof dab::FILE_MAGIC, std::ios_base::beg);

    size_t decl_offset = 0;
    _file.read(reinterpret_cast<char*>(&decl_offset), sizeof decl_offset);
    _file.seekg(decl_offset, std::ios_base::beg);

    uint32_t decl_count = 0;
    _file.read(reinterpret_cast<char*>(&decl_count), sizeof decl_count);

    std::vector<asset_decl> declarations(decl_count);
    for (auto& declaration : declarations) {
        constexpr uint32_t decl_header_size = sizeof asset_decl::offset + sizeof asset_decl::type + sizeof(uint8_t);

        char buffer[0xFF]{ 0 };
        _file.read(buffer, decl_header_size);

        declaration.offset = *reinterpret_cast<decltype(asset_decl::offset)*>(buffer);
        declaration.type = *reinterpret_cast<decltype(asset_decl::type)*>(buffer + sizeof(asset_decl::offset));

        const uint8_t str_len = *reinterpret_cast<uint8_t*>(buffer + sizeof(asset_decl::offset) + sizeof(asset_decl::type));

        _file.read(buffer, str_len);
        declaration.name.assign(buffer, buffer + str_len);
    }

    return declarations;
}

}