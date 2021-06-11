#pragma once

#ifndef DRY_UTIL_PERSISTENT_ARRAY_H
#define DRY_UTIL_PERSISTENT_ARRAY_H

#include <limits>
#include <algorithm>

#include "num.hpp"

namespace dry {

using persistent_index_type = u64_t;
static constexpr persistent_index_type persistent_index_null = (std::numeric_limits<persistent_index_type>::max)();

template<typename T>
class persistent_array {
public:
    using index_type = persistent_index_type;
    static constexpr index_type index_null = persistent_index_null;

private:
    union union_t {
        ~union_t() = delete;

        T type;
        index_type available;
    };

public:
    persistent_array(index_type capacity = 0) noexcept :
        _array{ nullptr },
        _capacity{ 0 },
        _head{ 0 },
        _available{ index_null },
        _available_capacity{ 0 }
    {
        reserve(capacity);
    }
    // NOTE : copies as is, doesn't compress allocated elements into a contiguous range
    persistent_array(const persistent_array& oth) :
        persistent_array{ oth._head }
    {
        if (oth._array == nullptr) {
            return;
        }

        if constexpr (std::is_trivially_copyable_v<T>) {
            std::copy(oth._array, oth._array + oth._head, _array);
        } else {
            auto copy_lambda = [this, &oth](index_type ind) {
                construct_type(&_array[ind].type, oth._array[ind].type);
            };
            oth.apply_to_range(copy_lambda);
            for (index_type av_it = oth._available; av_it != index_null; av_it = oth._array[av_it].available) {
                _array[av_it].available = oth._array[av_it].available;
            }
        }

        _head = oth._head;
        _available = oth._available;
        _available_capacity = oth._available_capacity;
    }

    persistent_array(persistent_array&& oth) noexcept :
        persistent_array{}
    {
        *this = std::move(oth);
    }

    ~persistent_array() {
        if (_array == nullptr) {
            return;
        }

        if constexpr (!std::is_trivially_destructible_v<T>) {
            auto destroy_lambda = [this](index_type ind) {
                _array[ind].type.~T();
            };
            apply_to_range(destroy_lambda);
        }
        ::operator delete(static_cast<void*>(_array));
    }

    void reserve(index_type capacity) {
        if (capacity <= _head) {
            return;
        }

        union_t* new_array = reinterpret_cast<union_t*>(::operator new(sizeof(union_t) * capacity));
        if (_array != nullptr) {
            if constexpr (std::is_trivially_move_assignable_v<T> && std::is_trivially_destructible_v<T>) {
                std::copy(_array, _array + _head, new_array);
            }
            else {
                auto move_lambda = [new_array, this](index_type ind) {
                    construct_type(&new_array[ind].type, std::move(_array[ind].type));
                    _array[ind].type.~T();
                };
                apply_to_range(move_lambda);
                for (index_type av_it = _available; av_it != index_null; av_it = _array[av_it].available) {
                    new_array[av_it].available = _array[av_it].available;
                }
            }
            ::operator delete(static_cast<void*>(_array));
        }
        _array = new_array;
        _capacity = capacity;
    }

    template<typename... Args>
    index_type emplace(Args&&... args) {
        index_type ret_pos = index_null;

        if (_available != index_null) {
            ret_pos = _available;
            _available = _array[_available].available;
            construct_type(&_array[ret_pos].type, std::forward<Args>(args)...);

            _available_capacity -= 1;
        }
        else {
            if (_head >= _capacity) {
                reserve(_capacity == 0 ? 1 : 2 * _capacity);
            }
            construct_type(&_array[_head].type, std::forward<Args>(args)...);

            ret_pos = _head;
            _head += 1;
        }
        return ret_pos;
    }

    void remove(index_type index) noexcept {
        _array[index].type.~T();
        _array[index].available = _available;

        _available = index;
        _available_capacity += 1;
    }

    const T& operator[](index_type index) const noexcept {
        return _array[index].type;
    }
    T& operator[](index_type index) noexcept {
        return _array[index].type;
    }

    index_type available_sparse() const noexcept {
        return _available_capacity;
    }
    index_type size() const noexcept {
        return _head - _available_capacity;
    }

    persistent_array& operator=(persistent_array&& oth) noexcept {
        // destroy
        if (_array != nullptr) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                auto destroy_lambda = [this](index_type ind) {
                    _array[ind].type.~T();
                };
                apply_to_range(destroy_lambda);
            }
            ::operator delete(static_cast<void*>(_array));
        }
        // move
        _array = oth._array;
        _capacity = oth._capacity;
        _head = oth._head;
        _available = oth._available;
        _available_capacity = oth._available_capacity;
        // null
        oth._array = nullptr;
        return *this;
    }

    persistent_array& operator=(const persistent_array& oth) {
        if (&oth == this) {
            return *this;
        }
        // tmp copy
        persistent_array tmp{ oth };
        // destroy
        if (_array != nullptr) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                auto destroy_lambda = [this](index_type ind) {
                    _array[ind].type.~T();
                };
                apply_to_range(destroy_lambda);
            }
            ::operator delete(static_cast<void*>(_array));
        }
        // move tmp
        *this = std::move(tmp);
        return *this;
    }

private:
    template<typename... Args>
    void construct_type(T* whereptr, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        ::new(static_cast<void*>(whereptr)) T(std::forward<Args>(args)...); // TODO : {} doesn't compile lol, some conversion issue
    }
    template<typename Functor>
    void apply_to_range(Functor functor) {
        index_type* available_array = new index_type[_available_capacity];

        index_type* curr_av_elem = available_array;
        for (index_type av_it = _available; av_it != index_null; av_it = _array[av_it].available) {
            *curr_av_elem = av_it;
            curr_av_elem += 1;
        }
        std::sort(available_array, available_array + _available_capacity);

        index_type arr_it = 0;
        for (auto i = 0u; i < _available_capacity; ++i) {
            const index_type fragment_limit = available_array[i];
            for (;arr_it < fragment_limit; ++arr_it) {
                functor(arr_it);
            }
            arr_it += 1;
        }
        for (; arr_it < _head; ++arr_it) {
            functor(arr_it);
        }
        delete[] available_array;
    }
    template<typename Functor>
    void apply_to_range(Functor functor) const {
        index_type* available_array = new index_type[_available_capacity];

        index_type* curr_av_elem = available_array;
        for (index_type av_it = _available; av_it != index_null; av_it = _array[av_it].available) {
            *curr_av_elem = av_it;
            curr_av_elem += 1;
        }
        std::sort(available_array, available_array + _available_capacity);

        index_type arr_it = 0;
        for (auto i = 0u; i < _available_capacity; ++i) {
            const index_type fragment_limit = available_array[i];
            for (;arr_it < fragment_limit; ++arr_it) {
                functor(arr_it);
            }
            arr_it += 1;
        }
        for (; arr_it < _head; ++arr_it) {
            functor(arr_it);
        }
        delete[] available_array;
    }

    union_t* _array;
    index_type _head;
    index_type _capacity;
    index_type _available;
    // for trivial types doesn't contribute anything, allows to dynamically alocate the available range only once
    index_type _available_capacity;
};

}

#endif
