#pragma once

#include "vkw/device/device.hpp"

namespace vkw
{
    class CmdPool : public Movable<CmdPool>
    {
    public:
        using Movable<CmdPool>::operator=;

        CmdPool() = default;
        CmdPool(CmdPool&&) = default;
        CmdPool(const Device* device, uint32_t queue);
        ~CmdPool();

        const VkHandle<VkCommandPool>& pool() const { return _pool; }

    private:
        friend class CmdBuffer;

        DevicePtr _device;

        VkHandle<VkCommandPool> _pool;
    };
}