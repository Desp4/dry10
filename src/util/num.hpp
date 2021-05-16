#pragma once

#ifndef DRY_NUM_H
#define DRY_NUM_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>

namespace dry {

using u8_t = std::uint8_t;
using i8_t = std::int8_t;
using u16_t = std::uint16_t;
using i16_t = std::int16_t;
using u32_t = std::uint32_t;
using i32_t = std::int32_t;
using u64_t = std::uint64_t;
using i64_t = std::int64_t;

using f32_t = float;
using f64_t = double;

using size_t = std::size_t;

using byte_vec = std::vector<std::byte>;
using byte_span = std::span<std::byte>;
using const_byte_span = std::span<const std::byte>;

}

#endif
