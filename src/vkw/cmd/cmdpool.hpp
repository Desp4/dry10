#pragma once

#include "../device/device.hpp"

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

        VkCommandPool pool() const;

    private:
        friend class CmdBuffer;

        DevicePtr _device;

        VkHandle<VkCommandPool> _pool;
    };
}