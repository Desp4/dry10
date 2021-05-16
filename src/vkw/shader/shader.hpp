#pragma once

#ifndef DRY_VK_SHADER_H
#define DRY_VK_SHADER_H

#include "vkw/vkw.hpp"

namespace dry::vkw {

class vk_shader_module {
public:
    vk_shader_module(const_byte_span bin, VkShaderStageFlagBits type);

    vk_shader_module() = default;
    vk_shader_module(vk_shader_module&& oth) { *this = std::move(oth); }
    ~vk_shader_module();

    VkShaderModule handle() const { return _module; }
    VkShaderStageFlagBits type() const { return _type; }

    vk_shader_module& operator=(vk_shader_module&&);

private:
    VkShaderModule _module = VK_NULL_HANDLE;
    VkShaderStageFlagBits _type;
};

}

#endif
