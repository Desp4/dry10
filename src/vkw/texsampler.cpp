#include "texsampler.hpp"

namespace dry::vkw {

vk_tex_sampler::vk_tex_sampler(const vk_device& device, u32_t mip_levels) :
    _device{ &device }
{
    // NOTE : some hardcoded values here
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = static_cast<f32_t>(mip_levels); // NOTE : remind me?
    vkCreateSampler(_device->handle(), &sampler_info, null_alloc, &_sampler);
}

vk_tex_sampler::~vk_tex_sampler() {
    if (_device != nullptr) {
        vkDestroySampler(_device->handle(), _sampler, null_alloc);
    }
}

vk_tex_sampler& vk_tex_sampler::operator=(vk_tex_sampler&& oth) {
    // destroy
    if (_device != nullptr) {
        vkDestroySampler(_device->handle(), _sampler, null_alloc);
    }
    // move
    _device = oth._device;
    _sampler = oth._sampler;
    // null
    oth._device = nullptr;
    return *this;
}

}
