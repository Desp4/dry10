#pragma once

#ifndef DRY_UTIL_UTIL_H
#define DRY_UTIL_UTIL_H

#include <utility>
#include "util/num.hpp"

namespace dry {

constexpr u32_t popcount(u32_t i) noexcept {
    // https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
    // TODO : replace with intrinsics or fall back to this
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

// taking T by value everywhere
template<typename T, T Null_Value>
struct nullable_primitive {
    T val;

    nullable_primitive() : val{ Null_Value } {}
    nullable_primitive(const nullable_primitive&) = default;
    nullable_primitive(nullable_primitive&& oth) { *this = std::move(oth); }
    nullable_primitive(T oth) : val{ oth } {}

    nullable_primitive& operator=(nullable_primitive&& oth) {
        if (this != &oth) {
            val = oth.val;
            oth.val = nullptr;
        }
        return *this;
    }
    nullable_primitive& operator=(const nullable_primitive& oth) {
        val = oth.val;
        return *this;
    }
    nullable_primitive& operator=(T oth) {
        val = oth;
        return *this;
    }

    operator T () const {
        return val;
    }
    T const* operator&() const {
        return &val;
    }
    T* operator&() {
        return &val;
    }


};

template<typename T>
struct nullable_ptr : nullable_primitive<T*, nullptr> {
    using nullable_primitive<T*, nullptr>::nullable_primitive;

    T& operator*() const {
        return *(this->val);
    }
    T* operator->() const {
        return this->val;
    }
};

}

#endif
