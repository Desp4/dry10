#include "pipeline_g.hpp"

#include <vector>

namespace vkw
{
    GraphicsPipeline::GraphicsPipeline(const Device* device, const RenderPass* renderPass, const Material& material, VkExtent2D extent) :
        _device(device),
        _renderPass(renderPass),
        _hasDepth(renderPass->depthEnabled()),
        _extent(extent)
    {
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages(material.shader.modules.size());
        for (int i = 0; i < shaderStages.size(); ++i)
        {
            shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[i].pNext = nullptr;
            shaderStages[i].flags = 0;
            shaderStages[i].stage = material.shader.stages[i];
            shaderStages[i].module = material.shader.modules[i];
            shaderStages[i].pName = "main";
            shaderStages[i].pSpecializationInfo = nullptr;
        }

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &material.shader.data.input.binding;
        vertexInputInfo.vertexAttributeDescriptionCount = material.shader.data.input.descriptors.size();
        vertexInputInfo.pVertexAttributeDescriptions = material.shader.data.input.descriptors.data();

        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
        assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        assemblyInfo.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = _extent.width;
        viewport.height = _extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = _extent;

        VkPipelineViewportStateCreateInfo viewportInfo{};
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = &viewport;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterInfo{};
        rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterInfo.depthClampEnable = VK_FALSE;
        rasterInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterInfo.lineWidth = 1.0f;
        rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterInfo.depthBiasEnable = VK_FALSE;
        rasterInfo.depthBiasConstantFactor = 0.0f;
        rasterInfo.depthBiasClamp = 0.0f;
        rasterInfo.depthBiasSlopeFactor = 0.0f;

        const auto numSamples = _renderPass.ptr->rasterSampleCount();
        VkPipelineMultisampleStateCreateInfo multisampleInfo{};
        multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleInfo.rasterizationSamples = numSamples;
        multisampleInfo.sampleShadingEnable = numSamples != VK_SAMPLE_COUNT_1_BIT;
        multisampleInfo.minSampleShading = 0.2f; // assume this value
        multisampleInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleInfo.alphaToOneEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
        depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilInfo.depthTestEnable = _hasDepth;
        depthStencilInfo.depthWriteEnable = _hasDepth;
        depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilInfo.depthBoundsTestEnable = VK_FALSE; // assume no bounds
        depthStencilInfo.minDepthBounds = 0.0f;
        depthStencilInfo.maxDepthBounds = 1.0f;
        depthStencilInfo.stencilTestEnable = VK_FALSE; // and no stencil

        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        blendAttachment.blendEnable = material.blendEnabled;
        blendAttachment.srcColorBlendFactor = material.blendEnabled ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_ONE;
        blendAttachment.dstColorBlendFactor = material.blendEnabled ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ZERO;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo blendInfo{};
        blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendInfo.logicOpEnable = VK_FALSE;
        blendInfo.logicOp = VK_LOGIC_OP_COPY;
        blendInfo.attachmentCount = 1;
        blendInfo.pAttachments = &blendAttachment;
        blendInfo.blendConstants[0] = 0.0f;
        blendInfo.blendConstants[1] = 0.0f;
        blendInfo.blendConstants[2] = 0.0f;
        blendInfo.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &material.shader.layout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        vkCreatePipelineLayout(_device.ptr->device(), &pipelineLayoutInfo, NULL_ALLOC, &_pipelineLayout.handle);

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &assemblyInfo;
        pipelineInfo.pTessellationState = nullptr;
        pipelineInfo.pViewportState = &viewportInfo;
        pipelineInfo.pRasterizationState = &rasterInfo;
        pipelineInfo.pMultisampleState = &multisampleInfo;
        pipelineInfo.pDepthStencilState = &depthStencilInfo;
        pipelineInfo.pColorBlendState = &blendInfo;
        pipelineInfo.pDynamicState = nullptr;
        pipelineInfo.layout = _pipelineLayout;
        pipelineInfo.renderPass = _renderPass.ptr->renderPass();
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // assume no recreation
        pipelineInfo.basePipelineIndex = -1;

        vkCreateGraphicsPipelines(_device.ptr->device(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL_ALLOC, &_pipeline.handle);
    }

    GraphicsPipeline::~GraphicsPipeline()
    {
        if (_device)
        {
            vkDestroyPipeline(_device.ptr->device(), _pipeline, NULL_ALLOC);
            vkDestroyPipelineLayout(_device.ptr->device(), _pipelineLayout, NULL_ALLOC);
        }
    }
    
    void GraphicsPipeline::bindPipeline(VkCommandBuffer buffer) const
    {
        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    }

    void GraphicsPipeline::bindDescriptorSets(VkCommandBuffer buffer, std::span<const VkDescriptorSet> sets) const
    {
        vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
            0, sets.size(), sets.data(), 0, nullptr);
    }
}