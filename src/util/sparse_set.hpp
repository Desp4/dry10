#pragma once

#include <vector>

#include "persistent_array.hpp"

namespace dry::util {

template<typename T>
class sparse_set {
public:
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    sparse_set(size_pt capacity = 0) :
        _sparse(capacity)
    {
        _dense.reserve(capacity);
        _sparse_ref.reserve(capacity);
    }

    template<typename... Args>
    size_pt emplace(Args&&... args) {
        const size_pt last_ind = _sparse.emplace(static_cast<size_pt>(_dense.size()));
        _dense.emplace_back(std::forward<Args>(args)...);
        _sparse_ref.push_back(last_ind);
        return last_ind;
    }
    void remove(size_pt index) {
        const size_pt dense_index = _sparse[index];
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

    void reserve(size_pt capacity) {
        _sparse.reserve(capacity);
        _dense.reserve(capacity);
        _sparse_ref.reserve(capacity);
    }
    void shrink_to_fit() {
        //_sparse.shrink_to_fit();
        _dense.shrink_to_fit();
    }

    size_pt size() const {
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

    const T& operator[](size_pt index) const {
        return _dense[_sparse[index]];
    }
    T& operator[](size_pt index) {
        return _dense[_sparse[index]];
    }

private:
    persistent_array<size_pt> _sparse;
    std::vector<T> _dense;
    std::vector<size_pt> _sparse_ref;
};

}