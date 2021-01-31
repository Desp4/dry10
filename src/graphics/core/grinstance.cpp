#include "grinstance.hpp"

#include <tuple>

#include <vkw/device/defconf.hpp>

#include "dbg/log.hpp"

namespace gr::core
{
    VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsInstance::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* userData)
    {
        if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            LOG_ERR("\033[32mVALIDATION LAYER\033[0m: %s", data->pMessage);
        return VK_FALSE;
    }

    GraphicsInstance::GraphicsInstance(wsi::NativeHandle window) :
        GraphicsInstance(window, EXTENSIONS, VAL_LAYERS, debugCallback, DEV_EXTENSIONS, FEATURES)
    {
    }

    GraphicsInstance::GraphicsInstance(wsi::NativeHandle window,
        std::span<const char* const> extensions, std::span<const char* const> layers,
        vkw::Instance::DebugCallback callback, std::span<const char* const> deviceExtensions,
        const VkPhysicalDeviceFeatures& features) :
#ifdef VKW_ENABLE_VAL_LAYERS
        _instance(extensions, layers, callback == nullptr ? debugCallback : callback),
#else
        _instance(EXTENSIONS, {}, nullptr),
#endif
        _surface(&_instance, window)
    {
        const auto physDevices = _instance.enumeratePhysicalDevices();
        VkPhysicalDevice physDevice = VK_NULL_HANDLE;
        for (const auto device : physDevices)
        {
            if (vkw::conf::deviceExtensionsSupport(device, deviceExtensions) &&
                vkw::conf::deviceFeaturesSupport(device, features) &&
                vkw::conf::deviceQueuesSupport(device, QUEUES) &&
                vkw::conf::swapFormatSupport(device, _surface.surface(), IMAGE_FORMAT, IMAGE_COLORSPACE) &&
                vkw::conf::swapPresentModeSupport(device, _surface.surface(), IMAGE_PRESENT_MODE))
            {
                physDevice = device;
                break;
            }
        }

        PANIC_ASSERT(physDevice != VK_NULL_HANDLE, "no suitable device found");
        // further down just figuring out queue indices up until the very end

        // NOTE : graphics needs a compute or sparse binding for transition img layout, idk which one
        uint32_t queueFamCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamCount, queueFamilies.data());

        // present, graphics, transfer
        int32_t tmpIndex = -1;
        std::vector<std::pair<uint32_t, uint32_t>*> familyInds(3);

        // get all queues, prefer dedicated, if not pick the first that satisfies the usage
        tmpIndex = vkw::conf::findPresentIndex(queueFamilies, physDevice, _surface.surface());
        PANIC_ASSERT(tmpIndex != -1, "could not find present queue");
        familyInds[0] = &_presentQueue;
        familyInds[0]->first = tmpIndex;

        tmpIndex = vkw::conf::findSeparateGraphicsIndex(queueFamilies);
        if (tmpIndex == -1) tmpIndex = vkw::conf::findAnyIndex(queueFamilies, VK_QUEUE_GRAPHICS_BIT);
        familyInds[1] = &_graphicsWorkerQueue;
        familyInds[1]->first = tmpIndex;

        tmpIndex = vkw::conf::findSeparateTransferIndex(queueFamilies);
        if (tmpIndex == -1) tmpIndex = vkw::conf::findAnyIndex(queueFamilies, VK_QUEUE_TRANSFER_BIT);
        familyInds[2] = &_transferWorketQueue;
        familyInds[2]->first = tmpIndex;

        // what it does: create queueInfo for each seaprate family, if multiple of one - count them for submission
        std::vector<vkw::Device::QueueInfo> queueInfos;
        for (int i = 0; i < familyInds.size(); ++i)
        {
            vkw::Device::QueueInfo queueInfo;
            queueInfo.queueFamilyIndex = familyInds[i]->first;
            queueInfo.queueCount = 1;

            familyInds[i]->second = 0;
            for (auto p = familyInds.begin() + i + 1; p != familyInds.end();)
            {              
                if (familyInds[i]->first == (*p)->first)
                {
                    if (queueInfo.queueCount == queueFamilies[(*p)->first].queueCount)
                    {
                        LOG_WRN("exceeded queue capacity of %i for family at index %i, using queue %i as fallback",
                            queueInfo.queueCount, (*p)->first, queueInfo.queueCount - 1);
                        (*p)->second = queueInfo.queueCount - 1;
                    }
                    else
                    {
                        (*p)->second = queueInfo.queueCount;
                        queueInfo.queueCount += 1;
                    }
                    p = familyInds.erase(p);
                }
                else
                {
                    ++p;
                }
            }
            queueInfo.priorities.resize(queueInfo.queueCount, 1.0f); // NOTE : all priorities to 1 for now
            queueInfos.push_back(std::move(queueInfo));
        }

        _device = vkw::Device(physDevice, queueInfos, deviceExtensions, features);
    }
}