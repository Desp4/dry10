#include "device.hpp"

#include <cmath>

namespace dry::vkw {

vk_device::vk_device(VkPhysicalDevice phys_device, std::span<const queue_info> queue_infos,
    std::span<const char* const> extensions, const VkPhysicalDeviceFeatures& features)
{
    _phys_device = phys_device;
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_infos.size());

    for (auto i = 0u; i < queue_infos.size(); ++i) {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = queue_infos[i].queue_family_index;
        queue_create_infos[i].queueCount = queue_infos[i].queue_count;
        queue_create_infos[i].pQueuePriorities = queue_infos[i].priorities.data();
        queue_create_infos[i].flags = 0;
        queue_create_infos[i].pNext = nullptr;
    }

    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pQueueCreateInfos = queue_create_infos.data();
    device_info.queueCreateInfoCount = static_cast<u32_t>(queue_create_infos.size());
    device_info.pEnabledFeatures = &features;
    device_info.ppEnabledExtensionNames = extensions.data();
    device_info.enabledExtensionCount = static_cast<u32_t>(extensions.size());
    // layers deprecated, skipping
    vkCreateDevice(_phys_device, &device_info, null_alloc, &_device);

    vkGetPhysicalDeviceMemoryProperties(_phys_device, &_mem_properties);
    vkGetPhysicalDeviceProperties(_phys_device, &_device_properties);
}

vk_device::~vk_device() {
    vkDestroyDevice(_device, null_alloc);
}

VkSurfaceCapabilitiesKHR vk_device::surface_capabilities(VkSurfaceKHR surface) const {
    VkSurfaceCapabilitiesKHR ret{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_phys_device, surface, &ret);
    return ret;
}

VkDeviceSize vk_device::pad_uniform_size(VkDeviceSize size) const {
    const auto min_align = _device_properties.limits.minUniformBufferOffsetAlignment;
    VkDeviceSize aligned = size;
    if (min_align != 0) {
        aligned = static_cast<VkDeviceSize>(std::ceilf(static_cast<f32_t>(min_align) / aligned) * min_align);
    } // Sascha Willems: (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1), does same thing, more cryptic
    return aligned;
}

u32_t vk_device::find_memory_type_index(u32_t type_filter, VkMemoryPropertyFlags properties) const {
    for (auto i = 0u; i < _mem_properties.memoryTypeCount; ++i) {
        if (type_filter & (1 << i) && properties ==
            (_mem_properties.memoryTypes[i].propertyFlags & properties))
        {
            return i;
        }
    }
    return (std::numeric_limits<u32_t>::max)();
}

void vk_device::wait_on_device() const {
    vkDeviceWaitIdle(_device);
}

vk_device& vk_device::operator=(vk_device&& oth) {
    // destroy
    vkDestroyDevice(_device, null_alloc);
    // move
    _device = oth._device;
    _phys_device = oth._phys_device;
    _mem_properties = std::move(oth._mem_properties);
    _device_properties = std::move(oth._device_properties);
    // null
    oth._device = VK_NULL_HANDLE;
    return *this;
}

}