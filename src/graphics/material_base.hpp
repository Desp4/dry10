#pragma once

#ifndef DRY_GR_MATERIAL_BASE_H
#define DRY_GR_MATERIAL_BASE_H

#include "util/num.hpp"

namespace dry {

class vulkan_renderer;

// TODO : can't handle refcounting yet, a problem
enum class material_resource_tag {
    texture
};

class material_base {
public:
    // args: renderer, resource id, resource tag
    // NOTE : id is u64
    using refcount_callback = void(*)(vulkan_renderer&, u64_t, material_resource_tag);

    material_base() noexcept = default;
    virtual ~material_base() = default;

    virtual void write_material_info(std::byte* dst) const = 0;
    virtual void perform_refcount(refcount_callback callback, vulkan_renderer& renderer) {};

private:
    friend class vulkan_renderer;

    u64_t pipeline_index;
    u64_t local_index;
};

}

#endif