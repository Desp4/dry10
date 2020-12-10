#include "cmdbuffer.hpp"

namespace vkw
{
    void CmdBuffer::beginBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlags usage)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = usage;
        vkBeginCommandBuffer(buffer, &beginInfo);
    }

    CmdBuffer::CmdBuffer(const CmdPool* pool) :
        _pool(pool)
    {
        VkCommandBufferAllocateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        bufferInfo.commandPool = _pool.ptr->pool();
        bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        bufferInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(_pool.ptr->_device.ptr->device(), &bufferInfo, &_buffer.handle);
    }

    CmdBuffer::~CmdBuffer()
    {
        if (_pool) vkFreeCommandBuffers(_pool.ptr->_device.ptr->device(), _pool.ptr->_pool, 1, &_buffer.handle);
    }

    void CmdBuffer::begin(VkCommandBufferUsageFlags usage)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = usage;
        vkBeginCommandBuffer(_buffer, &beginInfo);
    }

    const VkCommandBuffer& CmdBuffer::buffer() const
    {
        return _buffer.handle;
    }
}