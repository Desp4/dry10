#pragma once

#ifndef DRY_GR_MATERIAL_BASE_H
#define DRY_GR_MATERIAL_BASE_H

#include "util/num.hpp"

namespace dry {

class vulkan_renderer;

// TODO : can't handle refcounting yet, a problem
class material_base {
public:
    material_base() noexcept = default;
    virtual ~material_base() = default;

    virtual void write_material_info(std::byte* dst) const = 0;

private:
    friend class vulkan_renderer;

    u64_t pipeline_index;
    u64_t local_index;
};

}

#endif