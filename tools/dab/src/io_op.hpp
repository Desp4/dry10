#pragma once

#include <vector>
#include <concepts>

#ifndef DAB_IO_OP_H
#define DAB_IO_OP_H

using byte_vector = std::vector<std::byte>;

template<typename T>
concept Container = requires(T&& t) {
    std::begin(t);
    std::end(t);
};

template<Container T>
byte_vector& operator<<(byte_vector& dst_bytes, const T& container) {
    using value_type = std::decay_t<decltype(*std::begin(container))>;
    const auto size = sizeof(value_type) * (std::end(container) - std::begin(container));

    const auto dst_ind = dst_bytes.size();
    dst_bytes.resize(dst_bytes.size() + size);

    std::copy(std::begin(container), std::end(container), reinterpret_cast<value_type*>(dst_bytes.data() + dst_ind));
    return dst_bytes;
}

template<std::integral T>
byte_vector& operator<<(byte_vector& dst_bytes, T val) {
    const auto dst_ind = dst_bytes.size();
    dst_bytes.resize(dst_bytes.size() + sizeof(T));

    *reinterpret_cast<T*>(&dst_bytes[dst_ind]) = val;
    return dst_bytes;
}

#endif