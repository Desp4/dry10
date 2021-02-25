#pragma once

#include <tuple>

#include "component_set.hpp"

namespace dry::ecs {

template<typename... Component>
class component_view {
public:
    using pool_tuple = std::tuple<component_set<Component>*...>;
    class view_iterator final {
    public:
        using difference_type = typename std::iterator_traits<entity_set::iterator>::difference_type;
        using value_type = typename std::iterator_traits<entity_set::iterator>::value_type;
        using pointer = typename std::iterator_traits<entity_set::iterator>::pointer;
        using reference = typename std::iterator_traits<entity_set::iterator>::reference;
        using iterator_category = std::forward_iterator_tag;

        friend class component_view<Component...>;
        view_iterator() = default;

        pointer operator->() const {
            return &*_it;
        }
        reference operator*() const {
            return *_it;
        }
        view_iterator& operator++() {
            do {
                ++_it;
            } while (_it != _end && contains());
            return *this;
        }
        view_iterator& operator++(int) {
            view_iterator ret = *this;
            ++(*this);
            return ret;
        }
        bool operator==(const view_iterator& oth) const {
            return _it == oth._it;
        }
        bool operator!=(const view_iterator& oth) const {
            return _it != oth._it;
        }

    private:
        bool contains() const {
            return (std::get<component_set<Component>*>(*_pools)->contains(*_it) && ...);
        }

        view_iterator(entity_set::iterator beg, entity_set::iterator end, pool_tuple* pools) :
            _it(beg),
            _end(end),
            _pools(pools)
        {
            if (_it != _end && !contains()) {
                ++(*this);
            }
        }

        entity_set::iterator _it;
        entity_set::iterator _end;
        pool_tuple* _pools;
    };

    using iterator = view_iterator;

    component_view(component_set<Component>*... components) :
        _pools(components...)
    {
        auto search_lambda = [](auto& l, auto& r) {
            return l->size() < r->size();
        };
        _main_pool = (std::min)({ static_cast<entity_set*>(std::get<component_set<Component>*>(_pools))... }, search_lambda);
    }

    template<typename... Get_Comp>
    decltype(auto) get(entity ent) {
        if constexpr (sizeof...(Get_Comp) == 1) {
            return (std::get<component_set<Get_Comp>*>(_pools)->get(ent), ...);
        } else {
            return std::forward_as_tuple(std::get<component_set<Get_Comp>*>(_pools)->get(ent)...);
        }
    }

    view_iterator begin() {
        return { _main_pool->begin(), _main_pool->end(), &_pools };
    }
    view_iterator end() {
        return { _main_pool->end(), _main_pool->end(), &_pools };
    }

private:
    pool_tuple _pools;
    entity_set* _main_pool;
};

}