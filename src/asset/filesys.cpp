#include "filesys.hpp"

#include <array>
#include <unordered_map>

#include <zip.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "dbg/log.hpp"
#include "util/fs.hpp"

// TODO : return placeholders for assets, init them statically somewhere; not found shouldn't crash

namespace dry::asset {

static bool operator==(const mesh_source::vertex& l, const mesh_source::vertex& r) {
    return l.pos == r.pos && l.tex == r.tex;
}

struct vertex_hash {
    size_t operator()(const mesh_source::vertex& vertex) const noexcept {
        return std::hash<glm::vec3>{}(vertex.pos) ^ (std::hash<glm::vec2>{}(vertex.tex) << 1);
    }
};

filesystem::filesystem() : 
    _arch_handle{ nullptr }, 
    _arch_id{ 0 }
{
}

filesystem::~filesystem() {
    drop_archive();
}

bool filesystem::open_archive(const std::filesystem::path& path) {
    std::string path_str = path.string();
    int errnum{};

    zip_t* arch = zip_open(path_str.c_str(), ZIP_RDONLY, &errnum);
    if (arch == nullptr) {
        zip_error_t zip_err{};
        zip_error_init_with_code(&zip_err, errnum);
        LOG_ERR("Failed to open archive %s: %s", path_str.c_str(), zip_error_strerror(&zip_err));
        return false;
    }

    std::swap(_arch_handle, arch);
    std::swap(_arch_path, path_str);

    const auto id_file = read_file(".id");
    if (!id_file) {
        LOG_ERR("Failed to read ID, dropping archive");
        drop_archive();
        // revert members
        std::swap(_arch_handle, arch);
        std::swap(_arch_path, path_str);
        return false;
    }

    const u32_t new_id = *(reinterpret_cast<const u32_t*>(id_file->data()));
    if (new_id == std::numeric_limits<u32_t>::max()) {
        LOG_ERR("archive id is %zu which is a reserved id, dropping archive", new_id);
        drop_archive();
        // same, tedium
        std::swap(_arch_handle, arch);
        std::swap(_arch_path, path_str);
        return false;
    }

    if (arch != nullptr && (zip_close(arch) < 0)) {
        LOG_ERR("Error closing archive %s: %s", path_str.data(), zip_strerror(arch));
    }

    _arch_id = new_id;
    return true;
}

bool filesystem::drop_archive() {
    bool ret{};
    if (_arch_handle != nullptr && (ret = (zip_close(_arch_handle) < 0))) {
        LOG_ERR("Error closing archive %s: %s", _arch_path.data(), zip_strerror(_arch_handle));
        return ret;
    }
    return true;
}

template<>
mesh_source filesystem::load_asset(const std::string& name) {
    mesh_source ret_mesh;

    auto mesh_file = read_file(name + asset_source_ext_v<mesh_source>.data());
    if (!mesh_file) {
        LOG_ERR("Could not find asset %s.mesh", name);
        mesh_file = read_fallback(asset_source_ext_v<mesh_source>);
    }

    const u64_t vert_count = *reinterpret_cast<const u64_t*>(mesh_file->data());
    const u8_t tex_count = *reinterpret_cast<const u8_t*>(mesh_file->data() + sizeof(u64_t));
    // TODO : need different vertex types for mesh, as filesystem will shit itself if no tex coord is present
    std::vector<glm::vec3> vert_pos(vert_count);
    std::vector<glm::vec2> vert_tex(vert_count);

    const std::byte* mesh_it = mesh_file->data() + sizeof(u64_t) + sizeof(u8_t);

    std::copy(mesh_it, mesh_it + vert_count * sizeof(glm::vec3), reinterpret_cast<std::byte*>(vert_pos.data()));
    mesh_it += vert_count * sizeof(glm::vec3);
    std::copy(mesh_it, mesh_it + vert_count * sizeof(glm::vec2), reinterpret_cast<std::byte*>(vert_tex.data()));

    ret_mesh.indices.reserve(vert_pos.size());
    std::unordered_map<mesh_source::vertex, u32_t, vertex_hash> unique_verts;

    for (auto i = 0u; i < vert_pos.size(); ++i) {
        const mesh_source::vertex vertex{ .pos = vert_pos[i], .tex = vert_tex[i] };

        if (!unique_verts.contains(vertex)) {
            unique_verts[vertex] = static_cast<uint32_t>(ret_mesh.vertices.size());
            ret_mesh.vertices.push_back(vertex);
        }
        ret_mesh.indices.push_back(unique_verts[vertex]);
    }

    ret_mesh.vertices.shrink_to_fit();
    return ret_mesh;
}

template<>
texture_source filesystem::load_asset(const std::string& name) {
    auto texture_file = read_file(name + asset_source_ext_v<texture_source>.data());
    if (!texture_file) {
        LOG_ERR("Could not find asset %s.texture", name);
        texture_file = read_fallback(asset_source_ext_v<mesh_source>);
    }

    int w = 0, h = 0, ch = 0;
    stbi_uc* data = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(texture_file->data()), static_cast<int>(texture_file->size()),
        &w, &h, &ch, STBI_rgb_alpha
    );
    ch = 4; // TODO : forcing alpha

    const texture_source ret_tex{
        .pixel_data{ reinterpret_cast<const std::byte*>(data), reinterpret_cast<const std::byte*>(data + w * h * ch) },
        .width{ static_cast<u32_t>(w) },
        .height{ static_cast<u32_t>(h) },
        .channels{ static_cast<u8_t>(ch) }
    };
    stbi_image_free(data);
    return ret_tex;
}

template<>
shader_source filesystem::load_asset(const std::string& name) {
    using sh_names = asset_source_ext<shader_source>;
    shader_source ret_shader;

    // get all shader of same stage
    auto stage_lambda = [this, &ret_shader, &name](std::string_view type, shader_stage stage) {
        for (auto i = 0u;;++i) {
            auto shader = read_file(name + '.' + std::to_string(i) + type.data());
            if (!shader) {
                break;
            }

            ret_shader.oth_stages.push_back({ std::move(*shader), stage });
        }
    };

    stage_lambda(sh_names::vert_ext, shader_stage::vertex);
    stage_lambda(sh_names::frag_ext, shader_stage::fragment);

    const bool valid_shader =
        ret_shader.oth_stages.size() != 0 && // not empty
        ret_shader.oth_stages[0].stage == shader_stage::vertex && // vertex present
        std::find_if(ret_shader.oth_stages.begin(), ret_shader.oth_stages.end(), 
            [](auto& val) { return val.stage == shader_stage::fragment; }) != ret_shader.oth_stages.end(); // fragment present

    if (valid_shader) {
        // all good, move first vertex to dedicated member, swap and pop
        std::swap(*ret_shader.oth_stages.begin(), ret_shader.oth_stages.back());
        ret_shader.vert_stage = std::move(ret_shader.oth_stages.back());
        ret_shader.oth_stages.pop_back();
    }
    else {
        LOG_ERR("Could not find asset %s.shader", name);
        // NOTE : looking for frag and vert
        auto vert = read_fallback(std::string{ ".0" } + sh_names::vert_ext.data());
        auto frag = read_fallback(std::string{ ".0" } + sh_names::frag_ext.data());

        ret_shader.vert_stage.spirv = std::move(*vert);
        ret_shader.vert_stage.stage = shader_stage::vertex;

        ret_shader.oth_stages.clear();
        ret_shader.oth_stages.push_back({ std::move(*frag), shader_stage::fragment });
    }

    ret_shader.oth_stages.shrink_to_fit();
    return ret_shader;
}

filesystem& filesystem::operator=(filesystem&& oth) {
    _arch_handle = oth._arch_handle;
    _arch_id = oth._arch_id;
    _arch_path = std::move(oth._arch_path);

    oth._arch_handle = nullptr;
    return *this;
}

std::optional<byte_vec> filesystem::read_file(std::string_view name) {
    zip_stat_t stat{ .valid = 0 };

    auto arch_file = zip_fopen(_arch_handle, name.data(), 0);
    if (arch_file == nullptr) {
        return std::nullopt;
    }

    if ((zip_stat(_arch_handle, name.data(), 0, &stat) < 0) || !(stat.valid & ZIP_STAT_SIZE)) {
        zip_fclose(arch_file); // let it fail, everything is failing already
        return std::nullopt;
    }

    byte_vec ret_bin(stat.size);
    const bool read_err = (zip_fread(arch_file, ret_bin.data(), stat.size) < 0);
    zip_fclose(arch_file); // let if fail, again

    if (read_err) {
        return std::nullopt;
    }
    return ret_bin;
}

filesystem& filesystem::fallback_fs() {
    auto init_lambda = []() {
        const auto fallback_dir_full = g_exe_dir / _fallback_path;
        filesystem ret;
        if (!ret.open_archive(fallback_dir_full)) {
            LOG_ERR("No fallback asset archive %s present", _fallback_path.data());
            dbg::panic();
        }
        return ret;
    };

    static filesystem fallback = init_lambda();
    return fallback;
}

std::optional<byte_vec> filesystem::read_fallback(std::string_view ext) {
    // NOTE : Not sufficient to fetch all shader sources
    const auto ret = fallback_fs().read_file(std::string{ _fallback_name } + ext.data());
    if (!ret) {
        LOG_ERR("No fallback asset %s%s found", _fallback_name.data(), ext.data());
        dbg::panic();
    }
    return ret;
}

}
