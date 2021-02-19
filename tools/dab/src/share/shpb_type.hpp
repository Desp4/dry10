#pragma once

#include <cstdint>

namespace shpb {
    
enum class shader_type : uint8_t {
    none = 0x00,
    vertex = 0x01,
    fragment = 0x10,
    geometry = 0x08,
    compute = 0x20
};

constexpr const char FILE_MAGIC[] = { 'S', 'H', 'B', 'P' };

}