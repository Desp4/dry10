#pragma once

#include <type_traits>

#include "dab/dabasset.hpp"

namespace dry::asset {

template<class T>
concept asset_compatible = std::is_same_v<decltype(T::type), const dab::asset_type>;

template<asset_compatible Type>
struct asset_base {
    using type_t = Type;

    asset_base() : _hash(0) {}
    asset_base(size_t asset_hash) : _hash(asset_hash) {}

    size_t hash() const {
        return _hash;
    }

protected:
    size_t _hash;
};

}