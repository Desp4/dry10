#pragma once

#include <any>

#include "util/persistent_array.hpp"
#include "util/type.hpp"

#include "pool_view.hpp"

namespace dry::ecs {

class ec_registry {
    template<typename T>
    using component_type_id = util::type_id<T, ec_registry>;
    using pool_base = std::unique_ptr<entity_set>;

public:
    entity create() {
        return _entities.emplace(null_entity);
    }
    void destroy(entity ent) {
        for (auto& pool : _component_pools) {
            if (pool->contains(ent)) {
                pool->remove(ent);
            }
        }
        _entities.remove(ent);
    }
    template<typename Component, typename... Args>
    void attach(entity ent, Args&&... args) {
        const auto component_id = component_type_id<Component>::value();

        if (component_id >= _component_pools.size()) {
            _component_pools.resize(component_id + 1);
            _component_pools[component_id] = std::make_unique<component_set<Component>>();
        }

        auto& pool = *static_cast<component_set<Component>*>(_component_pools[component_id].get());
        pool.emplace(ent, std::forward<Args>(args)...);
    }
    template<typename Component>
    void detach(entity ent) {
        const auto component_id = component_type_id<Component>::value();
        _component_pools[component_id]->remove(ent);
    }

    template<typename... View_Comp>
    component_view<View_Comp...> view() {
        return { static_cast<component_set<View_Comp>*>(_component_pools[component_type_id<View_Comp>::value()].get())... };
    }

private:
    persistent_array<entity> _entities;
    std::vector<pool_base> _component_pools;
};

}