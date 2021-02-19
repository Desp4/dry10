#pragma once

#include <string>

namespace dab {

    enum class asset_type : uint8_t {
        none,
        texture,
        shader,
        mesh,
        VALUE_COUNT = mesh + 1
    };

    struct asset_decl {
        std::string name;
        size_t offset;
        asset_type type;
    };
}
