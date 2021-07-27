#pragma once

#ifndef DRY_UTIL_NUM_H
#define DRY_UTIL_NUM_H

#include <int.hpp>

#include <vector>
#include <span>

namespace dry {

using namespace dry_common;

using byte_vec = std::vector<std::byte>;
using byte_span = std::span<std::byte>;
using const_byte_span = std::span<const std::byte>;

}

#endif
