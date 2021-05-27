#pragma once

#ifndef DRY_UTIL_TRANS_HASHMAP_H
#define DRY_UTIL_TRANS_HASHMAP_H

#include <unordered_set>

namespace dry {

template<typename T, typename U, auto Fun>
concept hash_invokable = std::is_same_v<std::decay_t<std::invoke_result_t<decltype(Fun), T>>, U>;

template<typename Main, typename Hashed, auto Hasher> requires hash_invokable<Main, Hashed, Hasher>
struct transparent_hasher {
    using is_transparent = void;
    constexpr size_t operator()(const Main& val) const {
        if constexpr (std::is_pointer_v<Hashed>) {
            return reinterpret_cast<std::size_t>(std::invoke(Hasher, val));
        }
        else {
            return static_cast<std::size_t>(std::invoke(Hasher, val));
        }
        
    }
    constexpr size_t operator()(const Hashed& val) const {
        if constexpr (std::is_pointer_v<Hashed>) {
            return reinterpret_cast<std::size_t>(val);
        }
        else {
            return static_cast<std::size_t>(val);
        }
        
    }
};

template<typename Main, typename Hashed, auto Hasher> requires hash_invokable<Main, Hashed, Hasher>
struct transparent_comparator {
    using is_transparent = void;
    constexpr bool operator()(const Main& l, const Main& r) const {
        return std::invoke(Hasher, l) == std::invoke(Hasher, r);
    }
    constexpr bool operator()(Hashed l, const Main& r) const {
        return l == std::invoke(Hasher, r);
    }
    constexpr bool operator()(const Main& l, Hashed r) const {
        return std::invoke(Hasher, l) == r;
    }
};

template<typename Main, typename Hashed, auto Hasher>
using hashmap = std::unordered_set<Main,
    transparent_hasher<Main, Hashed, Hasher>,
    transparent_comparator<Main, Hashed, Hasher>
>;

}

#endif
