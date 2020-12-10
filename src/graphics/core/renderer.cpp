#include "renderer.hpp"

#include <cassert>

#include "../../vkw/device/defconf.hpp"

namespace gr::core
{
    VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* userData)
    {
        if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            fprintf(stderr, "Validation layer: %s\n", data->pMessage);
        return VK_FALSE;
    }

    Renderer::Renderer(wsi::NativeHandle windowHandle) :
#ifdef VKW_ENABLE_VAL_LAYERS
        _instance(_EXTENSIONS, _VAL_LAYERS, debugCallback),
#else
        _instance(_EXTENSIONS, std::span<const char* const>{}, nullptr),
#endif
        _surface(&_instance, windowHandle),
        _callback(defaultCallback)
    {
        const auto physDevices = _instance.enumeratePhysicalDevices();
        VkPhysicalDevice physDevice = VK_NULL_HANDLE;
        for (const auto device : physDevices)
        {
            if (vkw::conf::deviceExtensionsSupport(device, _DEV_EXTENSIONS) &&
                vkw::conf::deviceFeaturesSupport(device, _FEATURES) &&
                vkw::conf::deviceQueuesSupport(device, _QUEUES) &&
                vkw::conf::swapFormatSupport(device, _surface.surface(), _IMAGE_FORMAT, _IMAGE_COLORSPACE) &&
                vkw::conf::swapPresentModeSupport(device, _surface.surface(), _IMAGE_PRESENT_MODE))
            {
                physDevice = device;
                break;
            }
        }

        assert(physDevice != VK_NULL_HANDLE);
        uint32_t graphicsFamily, transferFamily;
        {
            // NOTE : graphics needs a compute or sparse binding for transition img layout, idk which one
            const auto graphics = vkw::conf::getQueueFamilyIndices(physDevice, VK_QUEUE_GRAPHICS_BIT);
            const auto transfer = vkw::conf::getQueueFamilyIndices(physDevice, VK_QUEUE_TRANSFER_BIT);
            assert(graphics.size());
            assert(transfer.size());

            transferFamily = transfer[0];
            for (const auto family_g : graphics)
            {
                VkBool32 ret;
                vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, family_g, _surface.surface(), &ret);
                if (ret)
                {
                    for (const auto family_t : transfer)
                    {
                        if (family_g != family_t)
                        {
                            transferFamily = family_t;
                            break;
                        }
                    }

                    graphicsFamily = family_g;
                    if (graphicsFamily != transferFamily)
                        break;
                }
            }
        }

        _device = vkw::Device(
            physDevice,
            graphicsFamily == transferFamily ? std::vector{ graphicsFamily } : std::vector{ graphicsFamily, transferFamily },
            _DEV_EXTENSIONS,
            _FEATURES);

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, _surface.surface(), &capabilities);
        _extent = capabilities.currentExtent;

        _swapchain = vkw::PresentSwapchain(&_device, _surface.surface(), _extent, capabilities.currentTransform,
            _IMAGE_COUNT, _IMAGE_FORMAT, _IMAGE_COLORSPACE, _IMAGE_PRESENT_MODE);
        _graphicsQueue = vkw::GraphicsQueue(&_device, graphicsFamily, 0); // TODO : FUTURE : queue index will go here
        _transferQueue = vkw::TransferQueue(&_device, transferFamily, 0);

        _renderPass = vkw::RenderPass(
            &_device, _extent,
            vkw::RenderPass_Color | vkw::RenderPass_Depth | vkw::RenderPass_MSAA,
            _IMAGE_FORMAT, _DEPTH_FORMAT, _MSAA_SAMPLE_COUNT);
        _renderPass.createFrameBuffers(_swapchain.swapViews());
        _cmdPool = vkw::CmdPool(&_device, 0); // TODO : hard coding queue
        for (auto& buf : _cmdBuffers)
            buf = vkw::CmdBuffer(&_cmdPool);
        vkGetDeviceQueue(_device.device(), 0, 0, &_presentQueue); // TODO : here too
    }

    void Renderer::createPipeline(const vkw::Material& material, const std::vector<vkw::Renderable>* renderables)
    {
        // not checking if present already
        _pipelines.emplace_back(&_device, &_renderPass, material, _extent, renderables);
    }

    void Renderer::setCallback(CallbackPtr callback)
    {
        _callback = callback;
    }

    void Renderer::setUserData(void* data)
    {
        _userData = data;
    }

    void Renderer::drawFrame()
    {
        vkw::PresentFrame frameInfo = _swapchain.acquireFrame();
        _callback(frameInfo.frameIndex, _userData);
        // TODO : waiting can be optimized significantly(i think :))
        //vkWaitForFences(_device.device(), 1, &_swapchain.getFrameFence(frameInfo.frameIndex), VK_TRUE, UINT64_MAX);
        const auto& buffer = _cmdBuffers[frameInfo.frameIndex].buffer();
        vkResetCommandBuffer(buffer, 0);

        _renderPass.startCmdPass(buffer, frameInfo.frameIndex);
        for (auto& pipeline : _pipelines)
        {
            pipeline.pipeline.bindPipeline(buffer);
            VkDeviceSize offsets[1]{ 0 };
            for (const auto& renderable : *pipeline.renderables)
            {
                vkCmdBindVertexBuffers(buffer, 0, 1, &renderable.staticData.vertexBuf.buffer(), offsets);
                vkCmdBindIndexBuffer(buffer, renderable.staticData.indexBuf.buffer(), 0, VK_INDEX_TYPE_UINT32);
                pipeline.pipeline.bindDescriptorSets(buffer, std::span{ renderable.descSets.sets().data() + frameInfo.frameIndex, 1 });
                vkCmdDrawIndexed(buffer, renderable.staticData.indexBuf.size() / sizeof(uint32_t), 1, 0, 0, 0);
            }
        }
        vkCmdEndRenderPass(buffer);
        vkEndCommandBuffer(buffer);

        _swapchain.submitFrame(_presentQueue, frameInfo, &buffer);
    }

    void Renderer::waitOnDevice() const
    {
        vkDeviceWaitIdle(_device.device());
    }

    const vkw::Device& Renderer::device() const
    {
        return _device;
    }

    const vkw::GraphicsQueue& Renderer::grQueue() const
    {
        return _graphicsQueue;
    }

    const vkw::TransferQueue& Renderer::trQueue() const
    {
        return _transferQueue;
    }
}