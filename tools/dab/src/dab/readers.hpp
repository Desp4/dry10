#pragma once

#include <fstream>
#include <vector>

#include "dab/asset.hpp"

struct AssetBlock
{
    std::ofstream file;
    std::vector<dab::AssetDecl> header;
};

namespace fs = std::filesystem;

int writeTexture(AssetBlock& block, const fs::path& path);
int writeShader(AssetBlock& block, const fs::path& path);
int writeMesh(AssetBlock& block, const fs::path& path);
