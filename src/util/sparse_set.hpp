#pragma once

#ifndef DRY_UTIL_SPARSE_SET_H
#define DRY_UTIL_SPARSE_SET_H

#include "persistent_array.hpp"

namespace dry {
// TODO : add noexcept
template<typename T>
class sparse_set {
public:
    using index_type = u64_t;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    sparse_set(index_type capacity = 0) :
        _sparse{ capacity }
    {
        _dense.reserve(capacity);
        _sparse_ref.reserve(capacity);
    }

    sparse_set(const sparse_set&) = default;
    sparse_set(sparse_set&&) = default;

    template<typename... Args>
    index_type emplace(Args&&... args) {
        const index_type last_ind = _sparse.emplace(static_cast<index_type>(_dense.size()));
        _dense.emplace_back(std::forward<Args>(args)...);
        _sparse_ref.push_back(last_ind);
        return last_ind;
    }
    void remove(index_type index) {
        const index_type dense_index = _sparse[index];
        _sparse.remove(index);

        std::swap(_dense[dense_index], _dense.back());
        std::swap(_sparse_ref[dense_index], _sparse_ref.back());

        _sparse[_sparse_ref[dense_index]] = dense_index;

        _dense.pop_back();
        _sparse_ref.pop_back();
    }
    void remove(iterator pos) {
        remove(_sparse[_sparse_ref[pos - _dense.begin()]]);
    }

    void reserve(index_type capacity) {
        _sparse.reserve(capacity);
        _dense.reserve(capacity);
        _sparse_ref.reserve(capacity);
    }
    void shrink_to_fit() {
        //_sparse.shrink_to_fit();
        _dense.shrink_to_fit();
    }

    index_type size() const {
        return _dense.size();
    }

    iterator begin() {
        return _dense.begin();
    }
    const_iterator begin() const {
        return _dense.cbegin();
    }
    const_iterator cbegin() const {
        return _dense.cbegin();
    }
    iterator end() {
        return _dense.end();
    }
    const_iterator end() const {
        return _dense.cend();
    }
    const_iterator cend() const {
        return _dense.cend();
    }
    T* data() {
        return _dense.data();
    }
    const T* data() const {
        return _dense.data();
    }

    const T& operator[](index_type index) const {
        return _dense[_sparse[index]];
    }
    T& operator[](index_type index) {
        return _dense[_sparse[index]];
    }
    // NOTE : works only when persistent_array copies as is
    sparse_set& operator=(sparse_set&) = default;
    sparse_set& operator=(sparse_set&&) = default;

private:
    persistent_array<index_type> _sparse;
    std::vector<T> _dense;
    std::vector<index_type> _sparse_ref;
};

}

#endif
