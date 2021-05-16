#pragma once

#include <limits>
#include <algorithm>

#include "util.hpp"

namespace dry::util {

using size_pt = uint32_t;
static constexpr size_pt size_pt_null = std::numeric_limits<size_pt>::max();

template<typename T>
class persistent_array {
public:
    union union_t {
        ~union_t() = delete;

        T type;
        size_pt available;
    };

    persistent_array(size_pt capacity = 0) :
        _array(nullptr),
        _capacity(0),
        _head(0),
        _available(size_pt_null),
        _available_capacity(0)
    {
        reserve(capacity);
    }
    persistent_array(const persistent_array& oth) :
        persistent_array(oth._head)
    {
        if (oth._array == nullptr) {
            return;
        }

        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(_array, oth._array, oth._head * sizeof(union_t));
        } else {
            auto copy_lambda = [this, &oth](size_pt ind) {
                construct_type(&_array[ind].type, oth._array[ind].type);
            };
            oth.apply_to_range(copy_lambda);
            for (size_pt av_it = oth._available; av_it != size_pt_null; av_it = oth._array[av_it].available) {
                _array[av_it].available = oth._array[av_it].available;
            }
        }

        _head = oth._head;
        _available = oth._available;
        _available_capacity = oth._available_capacity;
    }

    persistent_array(persistent_array&& oth) noexcept :
        _array(oth._array),
        _capacity(oth._capacity),
        _head(oth._head),
        _available(oth._available),
        _available_capacity(oth._available_capacity)
    {
        oth._array = nullptr;
    }

    ~persistent_array() {
        if (_array == nullptr) {
            return;
        }

        if constexpr (!std::is_trivially_destructible_v<T>) {
            auto destroy_lambda = [this](size_pt ind) {
                _array[ind].type.~T();
            };
            apply_to_range(destroy_lambda);
        }
        ::operator delete(static_cast<void*>(_array));
    }

    void reserve(size_pt capacity) {
        if (capacity <= _head) {
            return;
        }

        union_t* new_array = reinterpret_cast<union_t*>(::operator new(sizeof(union_t) * capacity));
        if (_array != nullptr) {
            if constexpr (std::is_trivially_move_assignable_v<T> && std::is_trivially_destructible_v<T>) {
                std::memcpy(new_array, _array, _head * sizeof(union_t));
            }
            else {
                auto move_lambda = [new_array, this](size_pt ind) {
                    construct_type(&new_array[ind].type, std::move(_array[ind].type));
                    _array[ind].type.~T();
                };
                apply_to_range(move_lambda);
                for (size_pt av_it = _available; av_it != size_pt_null; av_it = _array[av_it].available) {
                    new_array[av_it].available = _array[av_it].available;
                }
            }
            ::operator delete(static_cast<void*>(_array));
        }
        _array = new_array;
        _capacity = capacity;
    }

    template<typename... Args>
    size_pt emplace(Args&&... args) {
        size_pt ret_pos = size_pt_null;

        if (_available != size_pt_null) {
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

    void remove(size_pt index) noexcept {
        _array[index].type.~T();
        _array[index].available = _available;

        _available = index;
        _available_capacity += 1;
    }

    const T& operator[](size_pt index) const noexcept {
        return _array[index].type;
    }
    T& operator[](size_pt index) noexcept {
        return _array[index].type;
    }

    size_pt available_sparse() const noexcept {
        return _available_capacity;
    }
    size_pt size() const noexcept {
        return _head - _available_capacity;
    }

private:
    template<typename... Args>
    void construct_type(T* whereptr, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        ::new(static_cast<void*>(whereptr)) T(std::forward<Args>(args)...);
    }
    template<typename Functor>
    void apply_to_range(Functor functor) {
        size_pt* available_array = new size_pt[_available_capacity];

        size_pt* curr_av_elem = available_array;
        for (size_pt av_it = _available; av_it != size_pt_null; av_it = _array[av_it].available) {
            *curr_av_elem = av_it;
            curr_av_elem += 1;
        }
        std::sort(available_array, available_array + _available_capacity);

        size_pt arr_it = 0;
        for (auto i = 0u; i < _available_capacity; ++i) {
            const size_pt fragment_limit = available_array[i];
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
        size_pt* available_array = new size_pt[_available_capacity];

        size_pt* curr_av_elem = available_array;
        for (size_pt av_it = _available; av_it != size_pt_null; av_it = _array[av_it].available) {
            *curr_av_elem = av_it;
            curr_av_elem += 1;
        }
        std::sort(available_array, available_array + _available_capacity);

        size_pt arr_it = 0;
        for (auto i = 0u; i < _available_capacity; ++i) {
            const size_pt fragment_limit = available_array[i];
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
    size_pt _head;
    size_pt _capacity;
    size_pt _available;
    // for trivial types doesn't contribute anything, allows to dynamically alocate the available range only once
    size_pt _available_capacity;
};

}