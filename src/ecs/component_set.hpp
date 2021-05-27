#pragma once

#include <vector>
#include <memory>
#include <limits>

namespace dry::ecs {

using entity = uint32_t;
constexpr entity null_entity = std::numeric_limits<entity>::max();

// TODO : no static polymorphism, resorting to regular virtual inheritance
class entity_set {
public:
    using iterator = std::vector<entity>::reverse_iterator;
    using const_iterator = std::vector<entity>::const_reverse_iterator;

    virtual ~entity_set() = default;

    bool contains(entity ent) const noexcept {
        const uint32_t bucket = bucket_index(ent);
        return bucket < _sparse_ent.size() && _sparse_ent[bucket] && _sparse_ent[bucket][bucket_offset(ent)] != null_entity;
    }
    void emplace(entity ent) {
        secure_bucket(bucket_index(ent))[bucket_offset(ent)] = static_cast<entity>(_dense_ent.size());
        _dense_ent.push_back(ent);
    }
    void remove(entity ent) {
        // TODO : assume exists
        entity& dense_ind = _sparse_ent[bucket_index(ent)][bucket_offset(ent)];
        const entity back_ent = _dense_ent.back();

        std::swap(_dense_ent[dense_ind], _dense_ent.back());
        _dense_ent.pop_back();
        remove_component(dense_ind);

        _sparse_ent[bucket_index(back_ent)][bucket_offset(back_ent)] = dense_ind;
        dense_ind = null_entity;
    }

    uint32_t size() const {
        return static_cast<uint32_t>(_dense_ent.size());
    }

    iterator begin() {
        return _dense_ent.rbegin();
    }
    const_iterator begin() const {
        return _dense_ent.rbegin();
    }
    iterator end() {
        return _dense_ent.rend();
    }
    const_iterator end() const {
        return _dense_ent.rend();
    }

protected:
    virtual void remove_component(entity) = 0;

    static constexpr auto BUCKET_CAP = 1024u;
    static constexpr auto BUCKET_ENTITY_CAP = BUCKET_CAP / sizeof(entity);
    using bucket_t = std::unique_ptr<entity[]>;

    uint32_t bucket_index(entity ent) const noexcept {
        return ent / BUCKET_ENTITY_CAP;
    }
    uint32_t bucket_offset(entity ent) const noexcept {
        return ent % BUCKET_ENTITY_CAP; // for pow of 2 can just bitwise and
    }
    bucket_t& secure_bucket(uint32_t index) {
        if (index >= _sparse_ent.size()) {
            _sparse_ent.resize(index + 1);
        }

        if (!_sparse_ent[index]) {
            _sparse_ent[index].reset(new entity[BUCKET_ENTITY_CAP]);
            std::fill(_sparse_ent[index].get(), _sparse_ent[index].get() + BUCKET_ENTITY_CAP, null_entity);
        }
        return _sparse_ent[index];
    }

    std::vector<bucket_t> _sparse_ent;
    std::vector<entity> _dense_ent;
};

template<typename Component>
class component_set : public entity_set {
public:
    using iterator = typename std::vector<Component>::reverse_iterator;
    using const_iterator = typename std::vector<Component>::const_reverse_iterator;

    template<typename... Args>
    void emplace(entity ent, Args&&... args) {
        // NOTE : looked up in entt, emplace won't work for say initializer lists
        if constexpr (std::is_aggregate_v<Component>) {
            _components.push_back(Component{ std::forward<Args>(args)... });
        } else {
            _components.emplace_back(std::forward<Args>(args)...);
        }

        entity_set::emplace(ent);
    }
    Component& get(entity ent) {
        return _components[_sparse_ent[bucket_index(ent)][bucket_offset(ent)]];
    }
    const Component& get(entity ent) const {
        return _components[_sparse_ent[bucket_index(ent)][bucket_offset(ent)]];
    }

    iterator begin() {
        return _components.rbegin();
    }
    const_iterator begin() const {
        return _components.rbegin();
    }
    iterator end() {
        return _components.rend();
    }
    const_iterator end() const {
        return _components.rend();
    }

private:
    void remove_component(entity index) override {
        std::swap(_components[index], _components.back());
        _components.pop_back();
    }

    std::vector<Component> _components;
};
}