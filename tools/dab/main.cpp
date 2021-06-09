#include "dab.hpp"

#include <iostream>
#include <span>

#define ZIP_STATIC
#include <zip.h>

namespace fs = std::filesystem;

static std::vector<byte_vector> zip_buffers;

static void add_buffer_to_archive(zip_t* archive, std::string_view name, const void* data, std::uint64_t size) {
    auto src_buffer = zip_source_buffer(archive, data, size, 0);
    if (src_buffer == nullptr) {
        throw std::runtime_error{ std::string{ "Failed to add " } + name.data() + " to archive: " + zip_strerror(archive) };
    }

    if (zip_file_add(archive, name.data(), src_buffer, ZIP_FL_ENC_GUESS) < 0) {
        zip_source_free(src_buffer);
        throw std::runtime_error{ std::string{ "Failed to add " } + name.data() + " to archive: " + zip_strerror(archive) };
    }
    std::cout << name << " added to archive\n";
}

static void add_file_to_archive(zip_t* archive, const fs::path& path, const char* extension) {
    auto src_file = zip_source_file(archive, path.string().c_str(), 0, 0);
    if (src_file == nullptr) {
        throw std::runtime_error{ "Failed to add " + path.string() + " to archive: " + zip_strerror(archive) };
    }

    if (zip_file_add(archive, std::string{ path.stem().string() + '.' + extension }.c_str(), src_file, ZIP_FL_ENC_GUESS) < 0) {
        zip_source_free(src_file);
        throw std::runtime_error{ "Failed to add " + path.string() + " to archive: " + zip_strerror(archive) };
    }

    std::cout << path.filename().string() << " added to archive: " << extension << '\n';
}

int main(int argc, char** argv) {
    // command line parsing
    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        std::cout << "Usage: dab <dirs...> <output>\n";
        return 0;
    }
    if (argc < 3) {
        std::cerr << "Invalid usage, type --help or -h for commands\n";
        return -1;
    }

    const char* const out_name = argv[argc - 1];
    if (fs::exists(out_name)) {
        std::cout << "File " << out_name << " already exists\n";
        return 0;
    }

    // open zip
    int errnum{};
    auto archive_handle = zip_open(out_name, ZIP_CREATE, &errnum);
    if (archive_handle == nullptr) {
        zip_error_t zip_err{};
        zip_error_init_with_code(&zip_err, errnum);
        std::cerr << "Zip error for output file " << argv[argc - 1] << ": " << zip_error_strerror(&zip_err) << '\n';
        return -1;
    }  

    argc -= 2;
    argv += 1;

    // process files
    constexpr std::array<std::span<const std::string_view>, 3> asset_extensions{ shader_extensions, texture_extensions, mesh_extensions };
    constexpr std::array asset_parsers{ parse_shader, parse_texture, parse_mesh };
    
    static_assert(asset_extensions.size() == asset_parsers.size());
    constexpr std::uint32_t asset_type_count = static_cast<std::uint32_t>(asset_extensions.size());

    auto assure_extensions_lambda = [](std::span<const std::string_view> extensions, const std::string ext) -> bool {
        for (const auto& asset_ext : extensions) {
            const bool eq = std::equal(asset_ext.begin(), asset_ext.end(), ext.begin(), ext.end(),
                [](char l, char r) { return std::tolower(l) == std::tolower(r); }
            );

            if (eq) {
                return true;
            }
        }
        return false;
    };

    std::uint32_t total_files = 0;
    for (int i = 0; i < argc; ++i) {
        if (!fs::is_directory(argv[i])) {
            std::cerr << argv[i] << " is not a directory\n";
            return -1;
        }

        for (const auto& entry : fs::directory_iterator{ argv[i] }) {
            if (!entry.is_regular_file()) {
                continue;
            }

            const auto extension = entry.path().extension().string();
            for (auto j = 0u; j < asset_type_count; ++j) {
                if (assure_extensions_lambda(asset_extensions[j], extension)) {
                    auto ret_file = asset_parsers[j](entry.path());

                    for (auto& asset : ret_file) {
                        zip_buffers.push_back(std::move(asset.first));
                        const auto& last_buffer = zip_buffers.back();
                        add_buffer_to_archive(archive_handle, asset.second, last_buffer.data(), last_buffer.size());
                        total_files += 1;
                    }
                    break;
                }
            }
            // if not added - ignore
        }
    }

    if (zip_close(archive_handle) < 0) {
        std::cout << "Error creating a zip archive: " << zip_strerror(archive_handle) << '\n';
        return -1;
    }

    std::cout << "Added " << total_files << " assets to archive\n";
    return 0;
}