#pragma once

#ifndef DRY_UTIL_SPARSE_ARRAY_H
#define DRY_UTIL_SPARSE_ARRAY_H

#include <limits>
#include <type_traits>
#include <memory>
#include <vector>
#include <functional>
#include <algorithm>

#include "num.hpp"

namespace dry {

template<typename>
class sparse_array_iterator;

template<typename T>
class sparse_array {
public:
    using iterator = sparse_array_iterator<T>;
    using const_iterator = sparse_array_iterator<const T>;

    using index_t = u64_t;
    static constexpr index_t index_null = (std::numeric_limits<index_t>::max)();

    sparse_array(u64_t capacity = 0);
    sparse_array(const sparse_array&);
    sparse_array(sparse_array&&) noexcept;
    ~sparse_array();

    void reserve(u64_t capacity);
    void clear();
    u64_t size() const;

    template<typename... Args>
    index_t emplace(Args&&... args);
    void remove(index_t index);

    iterator begin();
    const_iterator begin() const;
    const_iterator cbegin() const;
    std::reverse_iterator<iterator> rbegin();
    std::reverse_iterator<const_iterator> rbegin() const;
    std::reverse_iterator<const_iterator> crbegin() const;

    iterator end();
    const_iterator end() const;
    const_iterator cend() const;
    std::reverse_iterator<iterator> rend();
    std::reverse_iterator<const_iterator> rend() const;
    std::reverse_iterator<const_iterator> crend() const;

    const T& operator[](index_t index) const noexcept;
    T& operator[](index_t index) noexcept;

    sparse_array& operator=(const sparse_array&);
    sparse_array& operator=(sparse_array&&) noexcept;

private:
    friend class iterator;
    friend class const_iterator;
    // store head at front
    std::vector<index_t> _available_queue;
    T* _arr;
    u64_t _capacity;
    index_t _first;
};

template<typename T>
class sparse_array_iterator {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = void;
    using value_type = std::remove_const_t<T>;
    using pointer = T*;
    using reference = T&;

    using index_t = typename sparse_array<value_type>::index_t;

    sparse_array_iterator() noexcept = default;

    index_t index() const noexcept;

    reference operator*() const noexcept;
    pointer operator->() const noexcept;

    sparse_array_iterator& operator++() noexcept;
    sparse_array_iterator operator++(int) noexcept;

    sparse_array_iterator& operator--() noexcept;
    sparse_array_iterator operator--(int) noexcept;

    template<typename T_Comp>
    friend bool operator==(const sparse_array_iterator<T_Comp>& l, const sparse_array_iterator<T_Comp>& r);
    template<typename T_Comp>
    friend bool operator!=(const sparse_array_iterator<T_Comp>& l, const sparse_array_iterator<T_Comp>& r);

private:
    friend class sparse_array<value_type>;

    using container_t = std::conditional_t<std::is_const_v<T>, const sparse_array<value_type>, sparse_array<value_type>>;
    using container_queue_t = decltype(sparse_array<value_type>::_available_queue);
    using container_queue_it_t = std::reverse_iterator<
        std::conditional_t<std::is_const_v<T>, typename container_queue_t::const_iterator, typename container_queue_t::iterator>
    >;

    static sparse_array_iterator begin_iterator(container_t& container);
    static sparse_array_iterator end_iterator(container_t& container);

    sparse_array_iterator(container_t& container, index_t index) noexcept;

    container_t* _container = nullptr;
    container_queue_it_t _queue_it = {};
    index_t _index = sparse_array<value_type>::index_null;
};




template<typename T>
sparse_array_iterator<T>::index_t sparse_array_iterator<T>::index() const noexcept {
    return _index;
}

template<typename T>
sparse_array_iterator<T>::reference sparse_array_iterator<T>::operator*() const noexcept {
    return (*_container)[_index];
}

template<typename T>
sparse_array_iterator<T>::pointer sparse_array_iterator<T>::operator->() const noexcept {
    return &(*_container)[_index];
}

template<typename T>
sparse_array_iterator<T>& sparse_array_iterator<T>::operator++() noexcept {
    _index += 1;
    while (_index == *_queue_it && _index != _container->_available_queue[0]) {
        _index += 1;
        ++_queue_it;
    }
    return *this;
}

template<typename T>
sparse_array_iterator<T> sparse_array_iterator<T>::operator++(int) noexcept {
    auto ret = *this;
    ++(*this);
    return ret;
}

template<typename T>
sparse_array_iterator<T>& sparse_array_iterator<T>::operator--() noexcept {
    _index -= 1;
    while (_index == *_queue_it && _index != _container->_first) {
        _index -= 1;
        --_queue_it;
    }
    return *this;
}

template<typename T>
sparse_array_iterator<T> sparse_array_iterator<T>::operator--(int) noexcept {
    auto ret = *this;
    ++(*this);
    return ret;
}

template<typename T>
bool operator==(const sparse_array_iterator<T>& l, const sparse_array_iterator<T>& r) {
    return l._container == r._container && l._queue_it == r._queue_it && l._index == r._index;
}

template<typename T>
bool operator!=(const sparse_array_iterator<T>& l, const sparse_array_iterator<T>& r) {
    return !(l == r);
}

template<typename T>
sparse_array_iterator<T> sparse_array_iterator<T>::begin_iterator(container_t& container) {
    sparse_array_iterator ret;
    ret._container = &container;
    ret._index = container._first;
    ret._queue_it = container._available_queue.rbegin();

    return ret;
}

template<typename T>
sparse_array_iterator<T> sparse_array_iterator<T>::end_iterator(container_t& container) {
    sparse_array_iterator ret;
    ret._container = &container;
    ret._index = container._available_queue[0];
    ret._queue_it = container._available_queue.rend() - 1;

    return ret;
}

template<typename T>
sparse_array_iterator<T>::sparse_array_iterator(container_t& container, index_t index) noexcept :
    _container{ &container },
    _queue_it{ std::upper_bound(
        container._available_queue.rbegin(),
        container._available_queue.rend(), index
    ) },
    _index{ index }
{
}



template<typename T>
sparse_array<T>::sparse_array(u64_t capacity) :
    _available_queue(1, 0),
    _arr{ nullptr },
    _capacity{ 0 },
    _first{ 0 }
{
    reserve(capacity);
}

template<typename T>
sparse_array<T>::sparse_array(const sparse_array& oth) {
    *this = oth;
}

template<typename T>
sparse_array<T>::sparse_array(sparse_array&& oth) noexcept {
    *this = std::move(oth);
}

template<typename T>
sparse_array<T>::~sparse_array() {
    if (_arr != nullptr) {
        clear();
        ::operator delete(static_cast<void*>(_arr));
    }
}

template<typename T>
void sparse_array<T>::reserve(u64_t capacity) {
    if (capacity <= _capacity) {
        return;
    }

    T* new_arr = static_cast<T*>(::operator new(sizeof(T) * capacity));
    if (_arr != nullptr) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::copy(_arr, _arr + _available_queue[0], new_arr);
        }
        else {
            for (auto p = begin(); p != end(); ++p) {
                std::construct_at(&new_arr[p.index()], std::move(_arr[p.index()]));
                _arr[p.index()].~T();
            }
        }
        ::operator delete(static_cast<void*>(_arr));
    }
    _arr = new_arr;
    _capacity = capacity;
}

template<typename T>
void sparse_array<T>::clear() {
    if constexpr (!std::is_trivially_destructible_v<T>) {
        for (auto& el : *this) {
            el.~T();
        }

        _available_queue.resize(1, 0);
        _first = 0;
    }
}

template<typename T>
u64_t sparse_array<T>::size() const {
    return _available_queue[0] - _available_queue.size() + 1;
}

template<typename T>
template<typename... Args>
sparse_array<T>::index_t sparse_array<T>::emplace(Args&&... args) {
    index_t ret_pos = index_null;

    if (_available_queue.size() != 1) {
        ret_pos = _available_queue.back();
        _available_queue.pop_back();
        std::construct_at(&_arr[ret_pos], std::forward<Args>(args)...);

        if (ret_pos < _first) {
            _first = ret_pos;
        }
    }
    else {
        if (_available_queue[0] >= _capacity) {
            reserve(_capacity == 0 ? 1 : 2 * _capacity);
        }
        std::construct_at(&_arr[_available_queue[0]], std::forward<Args>(args)...);

        ret_pos = _available_queue[0];
        _available_queue[0] += 1;
    }
    return ret_pos;
}

template<typename T>
void sparse_array<T>::remove(index_t index) {
    _arr[index].~T();

    auto it = std::upper_bound(_available_queue.begin() + 1, _available_queue.end(), index, std::greater<index_t>{});
    auto new_it = _available_queue.emplace(it, index);

    if (index == _first) {
        for (; new_it != _available_queue.begin(); --new_it) {
            if ((*(new_it - 1) - *new_it) != 1) {
                _first = *new_it + 1;
                return;
            }
        }
        _first = _available_queue[0];
    }
}

template<typename T>
sparse_array<T>::iterator sparse_array<T>::begin() {
    return iterator::begin_iterator(*this);
}

template<typename T>
sparse_array<T>::const_iterator sparse_array<T>::begin() const {
    return cbegin();
}

template<typename T>
sparse_array<T>::const_iterator sparse_array<T>::cbegin() const {
    return const_iterator::begin_iterator(*this);
}

template<typename T>
std::reverse_iterator<typename sparse_array<T>::iterator> sparse_array<T>::rbegin() {
    return std::reverse_iterator<iterator>{ begin() };
}

template<typename T>
std::reverse_iterator<typename sparse_array<T>::const_iterator> sparse_array<T>::rbegin() const {
    return crbegin();
}

template<typename T>
std::reverse_iterator<typename sparse_array<T>::const_iterator> sparse_array<T>::crbegin() const {
    return std::reverse_iterator<const_iterator>{ cbegin() };
}

template<typename T>
sparse_array<T>::iterator sparse_array<T>::end() {
    return iterator::end_iterator(*this);
}

template<typename T>
sparse_array<T>::const_iterator sparse_array<T>::end() const {
    return cend();
}

template<typename T>
sparse_array<T>::const_iterator sparse_array<T>::cend() const {
    return const_iterator::end_iterator(*this);
}

template<typename T>
std::reverse_iterator<typename sparse_array<T>::iterator> sparse_array<T>::rend() {
    return std::reverse_iterator<iterator>{ end() };
}

template<typename T>
std::reverse_iterator<typename sparse_array<T>::const_iterator> sparse_array<T>::rend() const {
    return crend();
}

template<typename T>
std::reverse_iterator<typename sparse_array<T>::const_iterator> sparse_array<T>::crend() const {
    return std::reverse_iterator<const_iterator>{ cend() };
}

template<typename T>
const T& sparse_array<T>::operator[](index_t index) const noexcept {
    return _arr[index];
}

template<typename T>
T& sparse_array<T>::operator[](index_t index) noexcept {
    return _arr[index];
}

template<typename T>
sparse_array<T>& sparse_array<T>::operator=(const sparse_array& oth) {
    if (&oth == this) {
        return *this;
    }

    clear();

    _available_queue = oth._available_queue;
    _first = oth._first;

    reserve(oth._available_queue[0]);

    for (auto p = oth.begin(); p != oth.end(); ++p) {
        std::construct_at(&_arr[p.index()], std::move(*p));
    }
    return *this;
}

template<typename T>
sparse_array<T>& sparse_array<T>::operator=(sparse_array&& oth) noexcept {
    clear();

    _available_queue = std::move(oth._available_queue);
    _first = oth._first;
    _capacity = oth._capacity;
    _arr = oth._arr;

    oth._arr = nullptr;

    return *this;
}


}

#endif