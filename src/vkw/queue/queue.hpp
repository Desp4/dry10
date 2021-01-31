#pragma once

#include "vkw/cmd/cmdbuffer.hpp"

namespace vkw
{
    // TODO : if doing multiple pools, ring buffers etc need to reflect that or just dump all queue functionality in free functions
    class Queue : public Movable<Queue>
    {
    public:
        using Movable<Queue>::operator=;

        Queue() = default;
        Queue(Queue&&) = default;
        Queue(const Device* device, uint32_t queueFamilyIndex, uint32_t queueIndex);

        void submitCmd(VkCommandBuffer cmdBuffer) const;

        // NOTE : need in these in renderer for allocating buffers and for swapchain only
        VkQueue queue() const { return _queue; }
        const CmdPool& pool() const { return _pool; }

    protected:
        VkQueue _queue;
        CmdPool _pool;
    };
}