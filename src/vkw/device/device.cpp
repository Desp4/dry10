#include "device.hpp"

#include <vector>

namespace vkw
{
    Device::Device(VkPhysicalDevice physDevice, std::span<const uint32_t> queues,
        std::span<const char* const> extensions, const VkPhysicalDeviceFeatures& features) :
        _physDevice(physDevice)
    {
        std::vector<VkDeviceQueueCreateInfo> queueInfos(queues.size());
        const float priority = 1.0f;

        for (int i = 0; i < queues.size(); ++i)
        {
            queueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfos[i].queueFamilyIndex = queues[i];
            queueInfos[i].queueCount = 1;
            queueInfos[i].pQueuePriorities = &priority;
            queueInfos[i].flags = 0;
            queueInfos[i].pNext = nullptr;
        }

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.pQueueCreateInfos = queueInfos.data();
        deviceInfo.queueCreateInfoCount = queueInfos.size();
        deviceInfo.pEnabledFeatures = &features;
        deviceInfo.ppEnabledExtensionNames = extensions.data();
        deviceInfo.enabledExtensionCount = extensions.size();
        // layers deprecated, skipping
        vkCreateDevice(_physDevice, &deviceInfo, NULL_ALLOC, &_device.handle);
    }

    Device::~Device()
    {
        vkDestroyDevice(_device, NULL_ALLOC);
    }

    VkDevice Device::device() const
    {
        return _device;
    }

    VkPhysicalDeviceMemoryProperties Device::memoryProperties() const
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(_physDevice, &memProperties);
        return memProperties;
    }
}