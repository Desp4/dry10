#pragma once

#include "asset/asset_src.hpp"

namespace dry::gr {

struct material {
    const asset::shader_asset* shader;
    std::vector<const asset::texture_asset*> textures;
};

}