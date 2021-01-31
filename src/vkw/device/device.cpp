#include "device.hpp"

#include <vector>

namespace vkw
{
    Device::Device(VkPhysicalDevice physDevice, std::span<const QueueInfo> queueInfos,
        std::span<const char* const> extensions, const VkPhysicalDeviceFeatures& features) :
        _physDevice(physDevice)
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(queueInfos.size());

        for (int i = 0; i < queueInfos.size(); ++i)
        {
            queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[i].queueFamilyIndex = queueInfos[i].queueFamilyIndex;
            queueCreateInfos[i].queueCount = queueInfos[i].queueCount;
            queueCreateInfos[i].pQueuePriorities = queueInfos[i].priorities.data();
            queueCreateInfos[i].flags = 0;
            queueCreateInfos[i].pNext = nullptr;
        }

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
        deviceInfo.pEnabledFeatures = &features;
        deviceInfo.ppEnabledExtensionNames = extensions.data();
        deviceInfo.enabledExtensionCount = extensions.size();
        // layers deprecated, skipping
        vkCreateDevice(_physDevice, &deviceInfo, NULL_ALLOC, &_device);
    }

    Device::~Device()
    {
        vkDestroyDevice(_device, NULL_ALLOC);
    }

    VkSurfaceCapabilitiesKHR Device::surfaceCapabilities(VkSurfaceKHR surface) const
    {
        VkSurfaceCapabilitiesKHR ret;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physDevice, surface, &ret);
        return ret;
    }

    VkPhysicalDeviceMemoryProperties Device::memoryProperties() const
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(_physDevice, &memProperties);
        return memProperties;
    }
}