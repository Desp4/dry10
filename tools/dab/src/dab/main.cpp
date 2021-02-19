#include <iostream>
#include <array>
#include <filesystem>

#include "readers.hpp"
#include "share/dab_type.hpp"

static asset_block _asset_block;

using asset_reader = bool (*)(asset_block&, const fs::path&);
static constexpr std::array<asset_reader, 3> _asset_readers{
    write_shader, write_texture, write_mesh
};

static void abort_file(const char* name) {
    _asset_block.file.close();
    fs::remove(name);
}

static bool write_asset(const fs::path& path) {
    for (const auto reader : _asset_readers) {
        if (reader(_asset_block, path)) {
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv) {
    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        std::cout <<
            "usage: dab { -d | -f } <args...> <output>\n" \
            "-d\targs are interpreted as directories(non-recursive)\n" \
            "-f\targs are interpreted as files\n";
        return 0;
    }
    if (argc < 4) {
        std::cerr << "invalid usage, type --help or -h for commands\n";
        return -1;
    }

    // mode flag, do both comparisons now, clearer to exit on failure just after
    const char mode = !strcmp(argv[1], "-f") | (!strcmp(argv[1], "-d") << 1);
    if (!mode) {
        std::cerr << "invalid usage, type --help or -h for commands\n";
        return -1;
    }

    argc -= 3;
    argv += 2;
    _asset_block.file.open(argv[argc], std::ios_base::binary);
    if (!_asset_block.file.is_open()) {
        std::cerr << "could not open output file " << argv[argc] << '\n';
        return -1;
    }

    
    _asset_block.file.write(dab::FILE_MAGIC, sizeof dab::FILE_MAGIC);
    _asset_block.file.seekp(sizeof(size_t), std::ios_base::cur);

    if (mode & 0x1) {
        for (int i = 0; i < argc; ++i) {
            if (!std::filesystem::is_regular_file(argv[i])) {
                if (!std::filesystem::exists(argv[i])) {
                    std::cerr << argv[i] << " does not exist\n";
                } else {
                    std::cerr << argv[i] << " is not a file\n";
                }
                std::cerr << "aborting\n";
                return -1;
            }

            if (!write_asset(argv[i])) {
                std::cerr << argv[i] << " is not a recognized asset\naborting\n";
                abort_file(argv[argc]);
                return -1;
            }

            std::cout << "asset " << argv[i] << " added to block\n";
        }
    } else {
        for (int i = 0; i < argc; ++i) {
            if (!fs::is_directory(argv[i])) {
                std::cerr << argv[i] << " is not a valid directory\naborting\n";
                abort_file(argv[argc]);
                return -1;
            }

            for (const auto& entry : fs::directory_iterator(argv[i])) {
                if (!entry.is_regular_file())
                    continue;

                if (!write_asset(entry.path())) {
                    std::cerr << entry.path() << " is not a recognized asset\naborting\n";
                    abort_file(argv[argc]);
                    return -1;
                }

                std::cout << "asset " << entry.path() << " added to block\n";
            }
        }
    }

    const size_t header_pos = _asset_block.file.tellp();
    const uint32_t header_count = _asset_block.header.size();

    _asset_block.file.write(reinterpret_cast<const char*>(&header_count), sizeof header_count);
    for (const auto& entry : _asset_block.header) {
        const uint8_t name_len = entry.name.size();
        _asset_block.file.write(reinterpret_cast<const char*>(&entry.offset), sizeof entry.offset);
        _asset_block.file.write(reinterpret_cast<const char*>(&entry.type), sizeof entry.type);
        _asset_block.file.write(reinterpret_cast<const char*>(&name_len), sizeof name_len);
        _asset_block.file.write(entry.name.c_str(), name_len);
    }
    _asset_block.file.seekp(sizeof dab::FILE_MAGIC, std::ios_base::beg);
    _asset_block.file.write(reinterpret_cast<const char*>(&header_pos), sizeof header_pos);

    return 0;
}