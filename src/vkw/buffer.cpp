#include "buffer.hpp"
#include "device/device.hpp"

namespace dry::vkw {

buffer_base::buffer_base(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) :
    _true_size(size)
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(device_main::device(), &buffer_info, NULL_ALLOC, &_buffer);

    // Get memory requirements
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device_main::device(), _buffer, &mem_requirements);

    _memory = device_memory(mem_requirements.size, device_main::find_memory_type_index(mem_requirements.memoryTypeBits, properties));
    vkBindBufferMemory(device_main::device(), _buffer, _memory.memory(), 0);
}

buffer_base::~buffer_base() {
    vkDestroyBuffer(device_main::device(), _buffer, NULL_ALLOC);
}

void buffer_base::write(const void* data, VkDeviceSize size) {
    _memory.write(data, size);
}

}