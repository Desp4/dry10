#pragma once

#include "../../vkw/device/surface.hpp"
#include "../../vkw/swapchain_p.hpp"
#include "../../vkw/queue/queue_graphics.hpp"
#include "../../vkw/queue/queue_transfer.hpp"
#include "../../vkw/pipeline_g.hpp"

#ifdef _DEBUG
  #ifndef VKW_ENABLE_VAL_LAYERS
    #define VKW_ENABLE_VAL_LAYERS
  #endif
#endif

namespace gr::core
{


    class Renderer
    {
    public:
        using CallbackPtr = void (*)(uint32_t, void*);

        static constexpr uint32_t                   _IMAGE_COUNT = 4;

        Renderer(wsi::NativeHandle windowHandle);

        void createPipeline(const vkw::Material& material, const std::vector<vkw::Renderable>* renderables);

        void setCallback(CallbackPtr callback);
        void setUserData(void* data);

        void drawFrame();
        void waitOnDevice() const;

        // TODO : don't like these
        const vkw::Device& device() const;
        const vkw::GraphicsQueue& grQueue() const;
        const vkw::TransferQueue& trQueue() const;

    private:
        struct Pipeline
        {
            Pipeline(const vkw::Device* device, const vkw::RenderPass* renderPass, const vkw::Material& material,
                VkExtent2D extent, const std::vector<vkw::Renderable>* irenderables) :
                mat_id(material.matID),
                pipeline(device, renderPass, material, extent),
                renderables(irenderables)
            {}

            uint32_t mat_id;
            vkw::GraphicsPipeline pipeline;
            const std::vector<vkw::Renderable>* renderables; // TODO : might too get invalidated
        };

        VkExtent2D _extent;

        vkw::Instance _instance;
        vkw::Surface _surface;
        vkw::Device _device;
        vkw::PresentSwapchain _swapchain;

        vkw::GraphicsQueue _graphicsQueue;
        vkw::TransferQueue _transferQueue;
        vkw::RenderPass _renderPass;
        VkQueue _presentQueue;

        vkw::CmdPool _cmdPool;
        std::array<vkw::CmdBuffer, _IMAGE_COUNT> _cmdBuffers;
        std::vector<Pipeline> _pipelines;

        CallbackPtr _callback;
        void* _userData;

        // statics
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type,
            const VkDebugUtilsMessengerCallbackDataEXT* data,
            void* userData);
        static void defaultCallback(uint32_t, void*) {}

        static constexpr std::array<const char*, 3> _EXTENSIONS{
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
#ifdef VKW_ENABLE_VAL_LAYERS
        static constexpr std::array<const char*, 1> _VAL_LAYERS{ "VK_LAYER_KHRONOS_validation" };
#endif
        static constexpr std::array<const char*, 1> _DEV_EXTENSIONS{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        static constexpr VkPhysicalDeviceFeatures   _FEATURES{ .sampleRateShading = VK_TRUE, .samplerAnisotropy = VK_TRUE };
        static constexpr VkQueueFlags               _QUEUES = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
        static constexpr VkFormat                   _IMAGE_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
        static constexpr VkColorSpaceKHR            _IMAGE_COLORSPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        static constexpr VkPresentModeKHR           _IMAGE_PRESENT_MODE = VK_PRESENT_MODE_MAILBOX_KHR;

        static constexpr VkFormat                   _DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
        static constexpr VkSampleCountFlagBits      _MSAA_SAMPLE_COUNT = VK_SAMPLE_COUNT_8_BIT;
    };
}