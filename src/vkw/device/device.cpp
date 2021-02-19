#include "device.hpp"
#include "dbg/log.hpp"

namespace dry::vkw {

void device_main::create(VkPhysicalDevice phys_device, std::span<const queue_info> queue_infos,
    std::span<const char* const> extensions, const VkPhysicalDeviceFeatures& features)
{
    PANIC_ASSERT(!_destroyed && _device == VK_NULL_HANDLE, "device should only be created once")

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
    device_info.queueCreateInfoCount = queue_create_infos.size();
    device_info.pEnabledFeatures = &features;
    device_info.ppEnabledExtensionNames = extensions.data();
    device_info.enabledExtensionCount = extensions.size();
    // layers deprecated, skipping
    vkCreateDevice(_phys_device, &device_info, NULL_ALLOC, &_device);
}

void device_main::destroy() {
    PANIC_ASSERT(!_destroyed && _device != VK_NULL_HANDLE, "device should only be destroyed once");
    vkDestroyDevice(_device, NULL_ALLOC);

    _destroyed = true;
}

VkSurfaceCapabilitiesKHR device_main::surface_capabilities(VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR ret{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_phys_device, surface, &ret);
    return ret;
}

VkPhysicalDeviceMemoryProperties device_main::memory_properties(){
    VkPhysicalDeviceMemoryProperties mem_properties{};
    vkGetPhysicalDeviceMemoryProperties(_phys_device, &mem_properties);
    return mem_properties;
}

uint32_t device_main::find_memory_type_index(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    const auto mem_properties = memory_properties();
    for (auto i = 0u; i < mem_properties.memoryTypeCount; ++i) {
        if (type_filter & (1 << i) && properties ==
            (mem_properties.memoryTypes[i].propertyFlags & properties))
        {
            return i;
        }
    }
    return UINT32_MAX;
}

void device_main::wait_on_device() {
    vkDeviceWaitIdle(_device);
}

}