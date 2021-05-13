#pragma once

#include <cstdint>

namespace dry::util {

using id_type = std::uint32_t;

template<typename Owner>
struct type_id_iterator {
    static id_type advance() {
        static id_type value{};
        return value++;
    }
};

template<typename T, typename Owner>
struct type_id {
    static id_type value() {
        static const id_type ret = type_id_iterator<Owner>::advance();
        return ret;
    }
};

}