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
    _arch_ind{ (std::numeric_limits<u32_t>::max)() },
    _arch_handle{ nullptr }
{
    u32_t total_assets = 0;

    for (const auto& dir_entry : std::filesystem::directory_iterator(g_exe_dir.string() + '/' + _root_dir.data())) {
        const auto& entry_path = dir_entry.path();
        if (dir_entry.is_regular_file() && entry_path.extension() == ".zip") {
            open_archive(entry_path.string());
            if (_arch_handle != nullptr) {
                const auto& entry_str = entry_path.string();

                const u32_t dab_index = static_cast<u32_t>(_dab_files.size());
                const u32_t num_assets = static_cast<u32_t>(zip_get_num_entries(_arch_handle, ZIP_FL_UNCHANGED));

                _dab_files.emplace_back(entry_str, total_assets);
                total_assets += num_assets;

                for (auto i = 0u; i < num_assets; ++i) {
                    auto arch_file = zip_fopen_index(_arch_handle, i, 0);
                    if (arch_file == nullptr) {
                        LOG_ERR("Could not open entry %i in archive %s", i, entry_str);
                        continue;
                    }

                    zip_stat_t stat{};
                    if ((zip_stat_index(_arch_handle, i, 0, &stat) < 0) || !(stat.valid & ZIP_STAT_NAME)) {
                        LOG_ERR("Could not get stats for entry %i in archive %s", i, entry_str);
                        zip_fclose(arch_file);
                        continue;
                    }

                    if (_asset_map.contains(stat.name)) {
                        LOG_ERR("Asset name %s from archive %s already present in filesystem", stat.name, entry_str);
                    }
                    else {
                        _asset_map[stat.name] = { dab_index, i };
                    }                   
                    zip_fclose(arch_file);
                }
                drop_archive();
            }
        }
    }
}

filesystem::~filesystem() {
    drop_archive();
}

void filesystem::open_archive(std::string_view path) {
    int errnum{};

    zip_t* arch = zip_open(path.data(), ZIP_RDONLY, &errnum);
    if (arch == nullptr) {
        zip_error_t zip_err{};
        zip_error_init_with_code(&zip_err, errnum);
        LOG_ERR("Failed to open archive %s: %s", path.data(), zip_error_strerror(&zip_err));
        return;
    }

    drop_archive();
    _arch_handle = arch;
}

void filesystem::open_archive(u32_t ind) {
    const auto& path = _dab_files[ind].path;
    const auto prev_handle = _arch_handle;
    open_archive(path);

    if (prev_handle != _arch_handle) {
        _arch_ind = ind;
    }
}

void filesystem::drop_archive() {
    if (_arch_handle != nullptr && zip_close(_arch_handle) < 0) {
        LOG_ERR("Error closing archive %s: %s", _dab_files[_arch_ind].path.data(), zip_strerror(_arch_handle));
    }
    _arch_handle = nullptr;
    _arch_ind = (std::numeric_limits<u32_t>::max)();
}

template<>
mesh_source filesystem::load_asset(hash_t hash) {
    mesh_source ret_mesh;

    auto mesh_file = read_file(hash);
    if (!mesh_file) {
        LOG_ERR("Could not find mesh asset hash %i", hash);
        return {};
    }

    const u64_t ind_count = *reinterpret_cast<const u64_t*>(mesh_file->data());
    const u64_t vert_count = *reinterpret_cast<const u64_t*>(mesh_file->data() + sizeof(u64_t));

    const std::byte* mesh_it = mesh_file->data() + sizeof(u64_t) * 2;
    ret_mesh.indices.resize(ind_count);
    ret_mesh.vertices.resize(vert_count);

    std::copy(mesh_it, mesh_it + ind_count * sizeof(u32_t), reinterpret_cast<std::byte*>(ret_mesh.indices.data()));
    mesh_it += ind_count * sizeof(u32_t);
    std::copy(mesh_it, mesh_it + vert_count * sizeof(mesh_source::vertex), reinterpret_cast<std::byte*>(ret_mesh.vertices.data()));

    return ret_mesh;
}

template<>
texture_source filesystem::load_asset(hash_t hash) {
    auto texture_file = read_file(hash);
    if (!texture_file) {
        LOG_ERR("Could not find texture asset hash %i", hash);
        return {};
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
shader_source filesystem::load_asset(hash_t hash) {
    // shader is multi stage, need to work backwards to iterate by name
    const auto asset_inds = consume_hash(hash);
    if (!asset_inds) {
        LOG_ERR("Could not find shader asset hash %i", hash);
        return {};
    }

    if (_arch_ind != asset_inds->dab_index) {
        open_archive(asset_inds->dab_index);
    }

    zip_stat_t stat{ .valid = 0 };
    if ((zip_stat_index(_arch_handle, asset_inds->asset_index, 0, &stat) < 0) || !(stat.valid & ZIP_STAT_NAME)) {
        LOG_ERR("Could not find shader asset hash %i", hash);
        return {};
    }
    const std::string name = std::filesystem::path{ stat.name }.stem().string();

    using sh_names = asset_source_ext<shader_source>;
    shader_source ret_shader;   

    // get all shader of same stage
    auto stage_lambda = [this, &ret_shader, &name](std::string_view type, shader_stage stage) {
        for (auto i = 0u;;++i) {
            const std::string shader_name = name + '.' + std::to_string(i) + type.data();
            if (!_asset_map.contains(shader_name)) {
                break;
            }

            auto shader = read_file(_asset_map[shader_name]);
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
material_source filesystem::load_asset(hash_t hash) {
    return {};
}

filesystem& filesystem::operator=(filesystem&& oth) {
    _arch_ind = oth._arch_ind;
    _arch_handle = oth._arch_handle;
    _dab_files = std::move(oth._dab_files);
    _asset_map = std::move(oth._asset_map);

    oth._arch_handle = nullptr;
    return *this;
}

std::optional<byte_vec> filesystem::read_file(asset_dab_location location) {
    if (_arch_ind != location.dab_index) {
        open_archive(location.dab_index);
    }

    zip_stat_t stat{ .valid = 0 };

    auto arch_file = zip_fopen_index(_arch_handle, location.asset_index, 0);
    if (arch_file == nullptr) {
        return std::nullopt;
    }

    if ((zip_stat_index(_arch_handle, location.asset_index, 0, &stat) < 0) || !(stat.valid & ZIP_STAT_SIZE)) {
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

std::optional<byte_vec> filesystem::read_file(hash_t hash) {
    const auto asset_inds = consume_hash(hash);
    if (!asset_inds) {
        return std::nullopt;
    }
    return read_file(*asset_inds);
}

std::optional<filesystem::asset_dab_location> filesystem::consume_hash(hash_t hash) const {
    auto dab_it = std::lower_bound(_dab_files.begin(), _dab_files.end(), hash,
        [](const dab_arch_data& el, hash_t hash) { return el.start_hash <= hash; }
    );

    if (dab_it == _dab_files.end() && _dab_files.size() == 0) {
        LOG_ERR("Asset hash %i not found", hash);
        return std::nullopt;
    }
    --dab_it;

    return asset_dab_location{
        .dab_index = static_cast<u32_t>(dab_it - _dab_files.begin()),
        .asset_index = hash - dab_it->start_hash
    };
}

}
