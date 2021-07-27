#include <span>

#include "importers.hpp"

struct asset_eq_range_comp {
    bool operator()(const std::string& str, const dab_asset& asset) const { return str < asset.name; }
    bool operator()(const dab_asset& asset, const std::string& str) const { return asset.name < str; }
};

static constexpr std::array<std::string_view, 1> shader_extensions{ ".glsl" };
static constexpr std::array<std::string_view, 2> texture_extensions{ ".png", ".jpg" };
static constexpr std::array<std::string_view, 1> mesh_extensions{ ".gltf" };
static constexpr u32_t asset_type_count = 3;

static constexpr std::array<std::span<const std::string_view>, asset_type_count> asset_extensions{
    shader_extensions, texture_extensions, mesh_extensions
};
static constexpr std::array<parsed_file(*)(const fs::path&), asset_type_count> asset_parsers{
    parse_shader, parse_texture, parse_mesh
};

static byte_vector& operator<<(byte_vector& dst_bytes, const byte_vector& container) {
    const auto dst_ind = dst_bytes.size();
    dst_bytes.resize(dst_bytes.size() + container.size());

    std::copy(container.begin(), container.end(), dst_bytes.data() + dst_ind);
    return dst_bytes;
}

static bool match_extension(std::span<const std::string_view> target_exts, std::string_view ext) {
    for (const auto& target_ext : target_exts) {
        const bool eq = std::equal(target_ext.begin(), target_ext.end(), ext.begin(), ext.end(),
            [](char l, char r) { return std::tolower(l) == std::tolower(r); });
        if (eq) {
            return true;
        }
    }
    return false;
}

static std::time_t utc_file_modify_time(const fs::directory_entry& entry) {
    namespace chr = std::chrono;
    return chr::system_clock::to_time_t(chr::clock_cast<chr::system_clock>(entry.last_write_time()));
}

static void write_dab_binaries(std::string_view filename, const dab_header& header, const byte_vector& bin) {
    const std::string tmp_name = std::string{ filename } + ".tmp";

    std::ofstream dab_out{ tmp_name, std::ios_base::binary | std::ios_base::out };
    write_dab_header(header, dab_out);
    dab_out.write(reinterpret_cast<const char*>(bin.data()), bin.size());
    dab_out.close();

    fs::rename(tmp_name, filename);
}

void create_dab(std::string_view filename, const dab_file_paths& assets, bool pedantic, bool overwite) {
    if (!overwite && fs::exists(filename)) {
        throw std::runtime_error{ std::string{ filename } + " already exists" };
    }

    dab_header header;
    byte_vector asset_bin;

    std::uint64_t offset = 0;
    for (const auto& asset : assets.files) {
        const auto ext = asset.extension().string();
        bool matched = false;

        for (auto i = 0u; i < asset_extensions.size(); ++i) {
            if (matched = match_extension(asset_extensions[i], ext)) {
                auto bins = asset_parsers[i](asset);
                for (auto& bin : bins) {
                    dab_asset new_asset;
                    new_asset.offset = asset_bin.size();
                    new_asset.size = bin.first.size();
                    new_asset.mod_time = static_cast<std::uint64_t>(utc_file_modify_time(fs::directory_entry{ asset }));
                    new_asset.name = std::move(bin.second);

                    const auto it = std::lower_bound(header.folders[i].begin(), header.folders[i].end(), new_asset.name, asset_eq_range_comp{});
                    header.folders[i].insert(it, std::move(new_asset));
                    asset_bin << bin.first;
                }
                break;
            }
        }

        if (pedantic && !matched) {
            throw std::runtime_error{ asset.string() + " file extension not supported" };
        }
    }

    std::ofstream dab_out{ filename, std::ios_base::binary | std::ios_base::out };
    write_dab_header(header, dab_out);
    dab_out.write(reinterpret_cast<const char*>(asset_bin.data()), asset_bin.size());
}

void update_dab(std::string_view filename, const dab_file_paths& assets, bool pedantic) {
    if (!fs::exists(filename)) {
        create_dab(filename, assets, pedantic, false);
        return;
    }

    std::ifstream dab_in{ filename, std::ios_base::binary | std::ios_base::in };
    const dab_header header = parse_dab_header(dab_in);
    dab_header new_header;
    byte_vector asset_bins;
    
    // if asset not changed and present - copy it to new_header, otherwise write it to new_header
    // redundant copies and writes because of that, but easy logic
    for (const auto& asset : assets.files) {
        const auto ext = asset.extension().string();
        bool matched = false;

        for (auto i = 0u; i < asset_extensions.size(); ++i) {
            if (matched = match_extension(asset_extensions[i], ext)) {
                std::string name = asset.stem().string();

                const auto it = std::lower_bound(header.folders[i].begin(), header.folders[i].end(), name, asset_eq_range_comp{});
                const std::time_t new_time = utc_file_modify_time(fs::directory_entry{ asset });

                // if asset not present or present but with different timestamp
                // NOTE : for meshes force update, need logic since they return multiple of these, shaders too
                if (i == folder_mesh || i == folder_shader || it == header.folders[i].end() || name != it->name || new_time != it->mod_time) {
                    auto bins = asset_parsers[i](asset);
                    for (auto& bin : bins) {
                        dab_asset new_asset;
                        new_asset.offset = asset_bins.size();
                        new_asset.size = bin.first.size();
                        new_asset.mod_time = new_time;
                        new_asset.name = std::move(bin.second);

                        const auto it = std::lower_bound(new_header.folders[i].begin(), new_header.folders[i].end(), new_asset.name, asset_eq_range_comp{});
                        new_header.folders[i].insert(it, std::move(new_asset));
                        asset_bins << bin.first;
                    }
                    break;
                } else {
                    const auto bin_offset = asset_bins.size();
                    const auto it_ins = std::lower_bound(new_header.folders[i].begin(), new_header.folders[i].end(), it->name, asset_eq_range_comp{});

                    new_header.folders[i].insert(it_ins, *it)->offset = bin_offset;

                    asset_bins.resize(asset_bins.size() + it->size);
                    dab_in.seekg(it->offset, std::ios_base::beg);
                    dab_in.read(reinterpret_cast<char*>(asset_bins.data() + bin_offset), it->size);
                }
                break;
            }
        }

        if (pedantic && !matched) {
            throw std::runtime_error{ asset.string() + " file extension not supported" };
        }
    }

    dab_in.close();
    write_dab_binaries(filename, new_header, asset_bins);
}

void add_dab(std::string_view filename, const dab_file_paths& assets, bool pedantic) {
    if (!fs::exists(filename)) {
        throw std::runtime_error{ std::string{ filename } + " does not exist" };
    }

    std::ifstream dab_in{ filename, std::ios_base::binary | std::ios_base::in };
    auto header = parse_dab_header(dab_in);
    // parse_header leaves position just after the header
    const std::uint64_t header_size = dab_in.tellg();

    dab_in.seekg(0, std::ios_base::end);
    byte_vector asset_bins(header_size - dab_in.tellg());
    dab_in.seekg(header_size, std::ios_base::beg);

    dab_in.read(reinterpret_cast<char*>(asset_bins.data()), asset_bins.size());

    for (const auto& asset : assets.files) {
        const auto ext = asset.extension().string();
        bool matched = false;

        for (auto i = 0u; i < asset_extensions.size(); ++i) {
            if (matched = match_extension(asset_extensions[i], ext)) {
                auto bins = asset_parsers[i](asset);
                for (auto& bin : bins) {
                    dab_asset new_asset;
                    new_asset.offset = asset_bins.size();
                    new_asset.size = bin.first.size();
                    new_asset.mod_time = utc_file_modify_time(fs::directory_entry{ asset });
                    new_asset.name = std::move(bin.second);

                    const auto it = std::lower_bound(header.folders[i].begin(), header.folders[i].end(), new_asset.name, asset_eq_range_comp{});
                    header.folders[i].insert(it, std::move(new_asset));

                    asset_bins << bin.first;
                }
                break;
            }
        }

        if (pedantic && !matched) {
            throw std::runtime_error{ asset.string() + " file extension not supported" };
        }
    }

    dab_in.close();
    write_dab_binaries(filename, header, asset_bins);
}
// TODO : fucked on shaders too, pack them into one asap
void remove_dab(std::string_view filename, const dab_file_paths& assets) {
    if (!fs::exists(filename)) {
        throw std::runtime_error{ std::string{ filename } + " does not exist" };
    }

    std::ifstream dab_in{ filename, std::ios_base::binary | std::ios_base::in };
    auto header = parse_dab_header(dab_in);

    const std::uint64_t header_size = dab_in.tellg();

    dab_in.seekg(0, std::ios_base::end);
    byte_vector asset_bins(header_size - dab_in.tellg());
    dab_in.seekg(header_size, std::ios_base::beg);

    dab_in.read(reinterpret_cast<char*>(asset_bins.data()), asset_bins.size());

    struct asset_eq_range_comp {
        bool operator()(const std::string& str, const dab_asset& asset) const { return str < asset.name; }
        bool operator()(const dab_asset& asset, const std::string& str) const { return asset.name < str; }
    };

    for (auto i = 0u; i < assets.files.size(); ++i) {
        std::uint32_t asset_index = 0;
        switch (assets.rem_types[i]) {
        case 's': asset_index = 0; break;
        case 't': asset_index = 1; break;
        case 'm': asset_index = 2; break;
        }

        const std::string name = assets.files[i].string();

        const auto it = std::lower_bound(header.folders[i].begin(), header.folders[i].end(), name, asset_eq_range_comp{});
        if (it == header.folders[i].end() || name != it->name) {
            throw std::runtime_error{ name + " is not present in the file" };
        }

        const auto offset = it->offset - header_size;

        asset_bins.erase(asset_bins.begin() + offset, asset_bins.begin() + offset + it->size);
        header.folders[i].erase(it);
    }

    dab_in.close();
    write_dab_binaries(filename, header, asset_bins);
}
