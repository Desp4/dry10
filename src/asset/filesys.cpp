#include "filesys.hpp"

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
    _curr_header{ (std::numeric_limits<u32_t>::max)() }
{
    u32_t total_assets = 0;

    for (const auto& dir_entry : std::filesystem::directory_iterator(g_exe_dir.string() + '/' + _root_dir.data())) {
        const auto& entry_path = dir_entry.path();
        if (dir_entry.is_regular_file() && entry_path.extension() == ".dab") {
            _dab_file.open(entry_path, std::ios_base::binary | std::ios_base::in);
            auto header = dab::parse_dab_header(_dab_file);

            _header_infos.emplace_back(entry_path, total_assets);

            for (const auto& folder : header.folders) {
                total_assets += static_cast<u32_t>(folder.size());
            }

            _headers.push_back(std::move(header));
            _dab_file.close();
        }
    }
}

void filesystem::open_archive(u32_t ind) {
    if (ind == _curr_header) {
        return;
    }

    _dab_file.open(_header_infos[ind].path, std::ios_base::binary | std::ios_base::in);
    _curr_header = ind;
}

void filesystem::drop_archive() {
    _dab_file.close();
    _curr_header = (std::numeric_limits<u32_t>::max)();
}

template<>
mesh_source filesystem::load_asset(u32_t header, u32_t pos) {
    mesh_source ret_mesh;

    const auto mesh_file = read_file<mesh_source>(header, pos);

    const u64_t ind_count = *reinterpret_cast<const u64_t*>(mesh_file.data());
    const u64_t vert_count = *reinterpret_cast<const u64_t*>(mesh_file.data() + sizeof(u64_t));

    const std::byte* mesh_it = mesh_file.data() + sizeof(u64_t) * 2;
    ret_mesh.indices.resize(ind_count);
    ret_mesh.vertices.resize(vert_count);

    std::copy(mesh_it, mesh_it + ind_count * sizeof(u32_t), reinterpret_cast<std::byte*>(ret_mesh.indices.data()));
    mesh_it += ind_count * sizeof(u32_t);
    std::copy(mesh_it, mesh_it + vert_count * sizeof(mesh_source::vertex), reinterpret_cast<std::byte*>(ret_mesh.vertices.data()));

    return ret_mesh;
}

template<>
texture_source filesystem::load_asset(u32_t header, u32_t pos) {
    const auto texture_file = read_file<texture_asset>(header, pos);

    int w = 0, h = 0, ch = 0;
    stbi_uc* data = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(texture_file.data()), static_cast<int>(texture_file.size()),
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
shader_source filesystem::load_asset(u32_t header, u32_t pos) {
    // shader is multi stage, need to work backwards to iterate by name
    const auto& folder = _headers[header].folders[asset_dab_ind<shader_source>::value];
    const auto& name = folder[pos].name;

    shader_source ret_shader;   

    // get all shader of same stage
    auto stage_lambda = [this, header, &ret_shader, &name, &folder](std::string_view type, shader_stage stage) {
        for (auto i = 0u;;++i) {
            const std::string shader_name = name + '.' + std::to_string(i) + type.data();
            const auto it = std::lower_bound(folder.begin(), folder.end(), shader_name, asset_eq_range_comp{});

            if (it == folder.end() || shader_name != it->name) {
                break;
            }

            auto shader = read_file<shader_source>(header, static_cast<u32_t>(it - folder.begin()));
            ret_shader.oth_stages.push_back({ std::move(shader), stage });
        }
    };

    stage_lambda(".vertex", shader_stage::vertex);
    stage_lambda(".fragment", shader_stage::fragment);

    const bool valid_shader =
        ret_shader.oth_stages.size() != 0 && // not empty
        ret_shader.oth_stages[0].stage == shader_stage::vertex; // vertex present

    if (valid_shader) {
        // all good, move first vertex to dedicated member, swap and pop
        std::swap(*ret_shader.oth_stages.begin(), ret_shader.oth_stages.back());
        ret_shader.vert_stage = std::move(ret_shader.oth_stages.back());
        ret_shader.oth_stages.pop_back();
    }
    else {
        LOG_ERR("Could not find asset %s.shader", name);
        return {};
    }

    ret_shader.oth_stages.shrink_to_fit();
    return ret_shader;
}

template<>
material_source filesystem::load_asset(u32_t header, u32_t pos) {
    return {};
}

}
