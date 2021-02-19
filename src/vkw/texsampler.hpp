#pragma once

#include "vkw.hpp"

namespace dry::vkw {

class tex_sampler : public movable<tex_sampler> {
public:
    using movable<tex_sampler>::operator=;

    tex_sampler() = default;
    tex_sampler(tex_sampler&&) = default;
    tex_sampler(uint32_t mip_levels);
    ~tex_sampler();

    const VkSampler& sampler() const {
        return _sampler;
    }

private:
    vk_handle<VkSampler> _sampler;
};

}