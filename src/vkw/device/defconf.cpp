#include "defconf.hpp"

namespace vkw::conf
{
    bool deviceExtensionsSupport(VkPhysicalDevice device, std::span<const char* const> extensions)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, supportedExtensions.data());

        // or could use a set or something
        bool extensionsPresent = true;
        for (auto p = extensions.begin(); p != extensions.end() && extensionsPresent; ++p)
        {
            for (const auto& supportedExtension : supportedExtensions)
            {
                if (extensionsPresent = !strcmp(supportedExtension.extensionName, *p))
                    break;
            }
        }
        return extensionsPresent;
    }

    bool deviceFeaturesSupport(VkPhysicalDevice device, const VkPhysicalDeviceFeatures& features)
    {
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        const VkBool32* pfeat = reinterpret_cast<const VkBool32*>(&features);
        const VkBool32* psfeat = reinterpret_cast<const VkBool32*>(&supportedFeatures);
        constexpr int limit = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

        bool featuresPresent = true;
        for (int i = 0; i < limit && featuresPresent; ++i)
            featuresPresent = !(*(pfeat + i)) || !(*(pfeat + i) ^ *(psfeat + i));
        return featuresPresent;
    }

    bool deviceQueuesSupport(VkPhysicalDevice device, VkQueueFlags queueFlags)
    {
        uint32_t queueFamCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(queueFamCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamCount, families.data());

        for (auto p = families.begin(); p != families.end() && queueFlags; ++p)
            queueFlags &= ~(queueFlags & p->queueFlags);
        return !queueFlags;
    }

    bool swapFormatSupport(VkPhysicalDevice device, VkSurfaceKHR surface, VkFormat format, VkColorSpaceKHR colorSpace)
    {
        uint32_t count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
        std::vector<VkSurfaceFormatKHR> supportedFormats(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, supportedFormats.data());

        for (const auto supportedFormat : supportedFormats)
        {
            if (supportedFormat.colorSpace == colorSpace && supportedFormat.format == format)
                return true;
        }
        return false;
    }

    bool swapPresentModeSupport(VkPhysicalDevice device, VkSurfaceKHR surface, VkPresentModeKHR presentMode)
    {
        uint32_t count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
        std::vector<VkPresentModeKHR> supportedModes(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, supportedModes.data());

        for (const auto supportedMode : supportedModes)
        {
            if (supportedMode == presentMode)
                return true;
        }
        return false;
    }

    std::vector<uint32_t> getQueueFamilyIndices(VkPhysicalDevice device, VkQueueFlags queueFamilies)
    {
        uint32_t queueFamCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(queueFamCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamCount, families.data());

        std::vector<uint32_t> ret;
        for (int i = 0; i < families.size(); ++i)
        {
            if (families[i].queueFlags & queueFamilies)
                ret.push_back(i);
        }
        return ret;
    }
}