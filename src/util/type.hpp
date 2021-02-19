#pragma once

#include <cstdint>

namespace dry::util {

using id_type = uint32_t;

template<uint32_t It_Instance>
struct type_id_iterator {
    static id_type advance() {
        static id_type value{};
        return value++;
    }
};

template<typename T, uint32_t It_Instance>
struct type_id {
    static id_type value() {
        static const id_type ret = type_id_iterator<It_Instance>::advance();
        return ret;
    }
};

}