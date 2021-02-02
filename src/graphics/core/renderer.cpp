#include "renderer.hpp"

namespace gr::core
{
    Renderer::Renderer(const GraphicsInstance* instance) :
        _instance(instance)
    {
        const auto capabilities = _instance->surfaceCapabilities();
        _surfaceExtent = capabilities.currentExtent;
        _imageCount = capabilities.maxImageCount == 0 ? capabilities.minImageCount + 1 :
            std::min<uint32_t>(capabilities.minImageCount + 1, capabilities.maxImageCount);

        _swapchain = vkw::PresentSwapchain(&_instance->device(), _instance->surface().surface(),
            _surfaceExtent, capabilities.currentTransform, _imageCount, // TODO : image count should be interpreted from capabilities
            GraphicsInstance::IMAGE_FORMAT, GraphicsInstance::IMAGE_COLORSPACE, GraphicsInstance::IMAGE_PRESENT_MODE);
        _renderPass = vkw::RenderPass(&_instance->device(), _surfaceExtent,
            vkw::RenderPass_Color | vkw::RenderPass_Depth | vkw::RenderPass_MSAA,
            GraphicsInstance::IMAGE_FORMAT, DEPTH_FORMAT, MSAA_SAMPLE_COUNT);

        _renderPass.createFrameBuffers(_swapchain.swapViews());

        const auto queueIndices = instance->presentQueue();
        _presentQueue = vkw::Queue(&_instance->device(), queueIndices.first, queueIndices.second);

        _cmdBuffers.resize(_imageCount);
        for (auto& buffer : _cmdBuffers)
            buffer = vkw::CmdBuffer(&_presentQueue.pool());
    }

    vkw::GraphicsPipeline Renderer::createPipeline(const Material& material, const vkw::DescriptorLayout& layout) const
    {
        std::vector<vkw::ShaderModule> shaderModules;
        shaderModules.reserve(material.shader->modules.size());
        for (const auto& shaderBin : material.shader->modules)
            shaderModules.emplace_back(&_instance->device(), shaderBin.data, static_cast<vkw::ShaderType>(shaderBin.stage));

        return vkw::GraphicsPipeline(&_instance->device(), &_renderPass, _surfaceExtent,
            shaderModules, material.shader->vkData(), std::span{ &layout.layout(), 1 });
    }

    RenderContext Renderer::beginFrame()
    {
        const uint32_t frameIndex = _swapchain.acquireFrame();
        const vkw::CmdBuffer& buffer = _cmdBuffers[frameIndex];
        vkResetCommandBuffer(buffer.buffer(), 0);

        _renderPass.startCmdPass(buffer.buffer(), frameIndex);

        return { frameIndex, &buffer };
    }

    void Renderer::submitFrame(uint32_t frameIndex)
    {
        const auto& buffer = _cmdBuffers[frameIndex].buffer();
        vkCmdEndRenderPass(buffer);
        vkEndCommandBuffer(buffer);
        _swapchain.submitFrame(_presentQueue.queue(), frameIndex, &buffer);
    }
}