#pragma once

#ifndef DRY_UTIL_SPARSE_TABLE_H
#define DRY_UTIL_SPARSE_TABLE_H

#include <limits>
#include <memory>
#include <type_traits>
#include <vector>
#include <algorithm>

#include "num.hpp"

namespace dry {

template<typename T>
class sparse_table {
public:
    using index_t = u64_t;
    static constexpr index_t index_null = (std::numeric_limits<index_t>::max)();

    sparse_table(u64_t capacity = 0);
    sparse_table(const sparse_table&);
    sparse_table(sparse_table&&) noexcept;
    ~sparse_table();

    void reserve(u64_t capacity);
    void clear();

    template<typename... Args>
    index_t emplace(Args&&... args);
    void remove(index_t index);

    const T& operator[](index_t index) const noexcept;
    T& operator[](index_t index) noexcept;

    sparse_table& operator=(const sparse_table&);
    sparse_table& operator=(sparse_table&&) noexcept;

private:
    union union_t {
        ~union_t() requires(!std::is_trivially_destructible_v<T>) = delete;
        ~union_t() = default;

        T value;
        index_t available;
    };

    static void apply_to_range(auto* self, auto fun);

    union_t* _arr;
    index_t _head;
    index_t _available;
    u64_t _capacity;
};



// impl
template<typename T>
sparse_table<T>::sparse_table(u64_t capacity) :
    _arr{ nullptr },
    _head{ 0 },
    _available{ index_null },
    _capacity{ 0 }
{
    reserve(capacity);
}

template<typename T>
sparse_table<T>::sparse_table(const sparse_table& oth) {
    *this = oth;
}

template<typename T>
sparse_table<T>::sparse_table(sparse_table&& oth) noexcept {
    *this = std::move(oth);
}

template<typename T>
sparse_table<T>::~sparse_table() {
    if (_arr != nullptr) {
        clear();
        ::operator delete(static_cast<void*>(_arr));
    }
}

template<typename T>
void sparse_table<T>::reserve(u64_t capacity) {
    if (capacity <= _capacity) {
        return;
    }

    union_t* new_arr = static_cast<union_t*>(::operator new(sizeof(union_t) * capacity));
    if (_arr != nullptr) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::copy(_arr, _arr + _head, new_arr);
        } else {
            auto move_l = [new_arr, this](index_t ind) {
                std::construct_at(&new_arr[ind].value, std::move(_arr[ind].value));
                _arr[ind].value.~T();
            };
            apply_to_range(this, move_l);

            for (auto av_it = _available; av_it != index_null; av_it = _arr[av_it].available) {
                new_arr[av_it].available = _arr[av_it].available;
            }
        }
        ::operator delete(static_cast<void*>(_arr));
    }
    _arr = new_arr;
    _capacity = capacity;
}

template<typename T>
void sparse_table<T>::clear() {
    if constexpr (!std::is_trivially_destructible_v<T>) {
        if (_arr != nullptr) {
            auto destroy_l = [this](index_t ind) { _arr[ind].value.~T(); };
            apply_to_range(this, destroy_l);
        }
    }
    _head = 0;
    _available = index_null;
}

template<typename T>
template<typename... Args>
sparse_table<T>::index_t sparse_table<T>::emplace(Args&&... args) {
    index_t ret_pos = index_null;

    if (_available != index_null) {
        ret_pos = _available;
        _available = _arr[_available].available;
        std::construct_at(&_arr[ret_pos].value, std::forward<Args>(args)...);
    } else {
        if (_head >= _capacity) {
            reserve(_capacity == 0 ? 1 : 2 * _capacity);
        }
        std::construct_at(&_arr[_head].value, std::forward<Args>(args)...);

        ret_pos = _head;
        _head += 1;
    }
    return ret_pos;
}

template<typename T>
void sparse_table<T>::remove(index_t index) {
    _arr[index].value.~T();
    _arr[index].available = _available;

    _available = index;
}

template<typename T>
const T& sparse_table<T>::operator[](index_t index) const noexcept {
    return _arr[index].value;
}

template<typename T>
T& sparse_table<T>::operator[](index_t index) noexcept {
    return _arr[index].value;
}

template<typename T>
sparse_table<T>& sparse_table<T>::operator=(const sparse_table& oth) {
    if (&oth == this) {
        return *this;
    }

    clear();
    reserve(oth._head);

    if constexpr (std::is_trivially_copyable_v<T>) {
        std::copy(oth._arr, oth._head, _arr);
    }
    else {
        auto copy_l = [this, &oth](index_t ind) { std::construct_at(&_arr[ind].value, oth[ind]); };
        apply_to_range(&oth, copy_l);

        for (auto av = oth._available; av != index_null; av = oth._arr[av]) {
            _arr[av] = oth._arr[av];
        }
    }

    _head = oth._head;
    _available = oth._available;

    return *this;
}

template<typename T>
sparse_table<T>& sparse_table<T>::operator=(sparse_table&& oth) noexcept {
    clear();

    _arr = oth._arr;
    _head = oth._head;
    _available = oth._available;
    _capacity = oth._capacity;

    oth._arr = nullptr;

    return *this;
}

template<typename T>
void sparse_table<T>::apply_to_range(auto* self, auto fun) {
    std::vector<index_t> av_arr;
    av_arr.reserve(self->_head / 4); // a guess, 1/4 is empty

    for (auto av_it = self->_available; av_it != index_null; av_it = self->_arr[av_it].available) {
        av_arr.insert(std::lower_bound(av_arr.begin(), av_arr.end(), av_it), av_it);
    }

    index_t arr_it = 0;
    for (auto frag_end : av_arr) {
        for (; arr_it < frag_end; ++arr_it) {
            fun(arr_it);
        }
        ++arr_it;
    }
    for (; arr_it < self->_head; ++arr_it) {
        fun(arr_it);
    }
}

}

#endif