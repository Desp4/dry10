#pragma once

#ifndef DRY_UTIL_UTIL_H
#define DRY_UTIL_UTIL_H

#include <utility>
#include <array>
#include "util/num.hpp"

namespace dry {

constexpr u32_t popcount(u32_t i) noexcept {
    // https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
    // TODO : replace with intrinsics or fall back to this
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

template<typename T, std::size_t N, typename Functor>
constexpr auto generate_array(const std::array<T, N>& range, Functor fun) {
    using U = decltype(fun(std::declval<T>()));
    std::array<U, N> ret;
    for (auto i = 0u; i < N; ++i) {
        ret[i] = fun(range[i]);
    }
    return ret;
}

}

#endif
