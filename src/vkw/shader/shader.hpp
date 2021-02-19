#pragma once

#include <span>

#include "vkw/vkw.hpp"

namespace dry::vkw {

enum class shader_type : uint32_t {
    vertex   = VK_SHADER_STAGE_VERTEX_BIT,
    geometry = VK_SHADER_STAGE_GEOMETRY_BIT,
    fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
    compute  = VK_SHADER_STAGE_COMPUTE_BIT
};

class shader_module : public movable<shader_module> {
public:
    using movable<shader_module>::operator=;

    shader_module() = default;
    shader_module(shader_module&&) = default;
    shader_module(std::span<const uint32_t> bin, shader_type type);
    ~shader_module();

    const VkShaderModule& sh_module() const {
        return _module;
    }
    const shader_type& type() const {
        return _type;
    }

private:
    vk_handle<VkShaderModule> _module;
    shader_type _type;
};

}
