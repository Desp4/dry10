#pragma once

#include <vector>

#include "cmdpool.hpp"

namespace vkw
{
    class CmdBuffer : public Movable<CmdBuffer>
    {
    public:
        using Movable<CmdBuffer>::operator=;

        static void beginBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlags usage);
        
        CmdBuffer() = default;
        CmdBuffer(CmdBuffer&&) = default;
        CmdBuffer(const CmdPool* pool);
        ~CmdBuffer();

        void begin(VkCommandBufferUsageFlags usage);

        const VkHandle<VkCommandBuffer>& buffer() const { return _buffer; }

    private:
        NullablePtr<const CmdPool> _pool;

        VkHandle<VkCommandBuffer> _buffer;
    };
}