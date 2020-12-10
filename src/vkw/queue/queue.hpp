#pragma once

#include "../cmd/cmdbuffer.hpp"

namespace vkw
{
    class Queue : public Movable<Queue>
    {
    public:
        using Movable<Queue>::operator=;

        Queue() = default;
        Queue(Queue&&) = default;
        Queue(const Device* device, uint32_t queueFamilyIndex, uint32_t queueIndex);

        void submitCmd(VkCommandBuffer cmdBuffer) const;

    protected:
        DevicePtr _device;
        VkQueue _queue;

        CmdPool _pool;
    };
}