#pragma once

#include <fstream>
#include <vector>

#include "dab/asset.hpp"

namespace fs = std::filesystem;

struct asset_block {
    std::ofstream file;
    std::vector<dab::asset_decl> header;
};

bool write_texture(asset_block& block, const fs::path& path);
bool write_shader(asset_block& block, const fs::path& path);
bool write_mesh(asset_block& block, const fs::path& path);
