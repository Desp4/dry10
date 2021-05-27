#pragma once

#ifndef DRY_VK_TEXSAMPLER_H
#define DRY_VK_TEXSAMPLER_H

#include "vkw/device/device.hpp"

namespace dry::vkw {

class vk_tex_sampler {
public:
    vk_tex_sampler(const vk_device& device, u32_t mip_levels);

    vk_tex_sampler() = default;
    vk_tex_sampler(vk_tex_sampler&& oth) { *this = std::move(oth); }
    ~vk_tex_sampler();

    VkSampler handle() const { return _sampler; }

    vk_tex_sampler& operator=(vk_tex_sampler&&);

private:
    const vk_device* _device = nullptr;
    VkSampler _sampler = VK_NULL_HANDLE;
};

}

#endif
