#pragma once

#include "device/device.hpp"

namespace vkw
{
    class TexSampler : public Movable<TexSampler>
    {
    public:
        using Movable<TexSampler>::operator=;

        TexSampler() = default;
        TexSampler(TexSampler&&) = default;
        TexSampler(const Device* device, uint32_t mipLevels);
        ~TexSampler();

        VkSampler sampler() const;

    private:
        DevicePtr _device;

        VkHandle<VkSampler> _sampler;
    };
}