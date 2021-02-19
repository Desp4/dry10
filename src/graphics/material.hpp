#pragma once

#include "asset/shaderasset.hpp"
#include "asset/texasset.hpp"

namespace dry::gr {

struct material {
    const asset::shader_asset* shader;
    std::vector<const asset::texture_asset*> textures;
};

}