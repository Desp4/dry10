#pragma once

#include <array>

#include "vkw/device/surface.hpp"
#include "vkw/device/device.hpp"

#include "vkw/queue/queue_graphics.hpp"
#include "vkw/queue/queue_transfer.hpp"

#ifdef _DEBUG
  #ifndef VKW_ENABLE_VAL_LAYERS
    #define VKW_ENABLE_VAL_LAYERS
  #endif
#endif

namespace gr::core
{
    class GraphicsInstance
    {
    public:
        constexpr static std::array<const char*, 3> EXTENSIONS{
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
        constexpr static std::array<const char*, 1> VAL_LAYERS{
            "VK_LAYER_KHRONOS_validation" };
        constexpr static std::array<const char*, 1> DEV_EXTENSIONS{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        constexpr static VkPhysicalDeviceFeatures   FEATURES{
            .sampleRateShading = VK_TRUE, .samplerAnisotropy = VK_TRUE };

        constexpr static VkFormat         IMAGE_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
        constexpr static VkColorSpaceKHR  IMAGE_COLORSPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        constexpr static VkPresentModeKHR IMAGE_PRESENT_MODE = VK_PRESENT_MODE_MAILBOX_KHR;

        GraphicsInstance(wsi::NativeHandle window);
        // NOTE : TODO : eh, also some extensions would need additional queue families and you do not consider that yet
        GraphicsInstance(wsi::NativeHandle window,
            std::span<const char* const> extensions, std::span<const char* const> layers,
            vkw::Instance::DebugCallback callback, std::span<const char* const> deviceExtensions,
            const VkPhysicalDeviceFeatures& features);

        VkSurfaceCapabilitiesKHR surfaceCapabilities() const { return _device.surfaceCapabilities(_surface.surface()); }

        std::pair<uint32_t, uint32_t> presentQueue() const { return _presentQueue; }
        std::pair<uint32_t, uint32_t> graphicsQueue() const { return _graphicsWorkerQueue; }
        std::pair<uint32_t, uint32_t> transferQueue() const { return _transferWorketQueue; }

        const vkw::Device& device() const { return _device; }
        const vkw::Surface& surface() const { return _surface; }

    private:
        constexpr static VkQueueFlags QUEUES = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type,
            const VkDebugUtilsMessengerCallbackDataEXT* data,
            void* userData);

        vkw::Instance _instance;
        vkw::Surface _surface;
        vkw::Device _device;

        // family and index
        std::pair<uint32_t, uint32_t> _presentQueue;
        std::pair<uint32_t, uint32_t> _graphicsWorkerQueue;
        std::pair<uint32_t, uint32_t> _transferWorketQueue;
    };
}