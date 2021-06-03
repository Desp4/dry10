#include "pipeline_g.hpp"

namespace dry::vkw {

vk_pipeline_graphics::vk_pipeline_graphics(const vk_device& device,
    const vk_render_pass& pass, VkExtent2D extent, std::span<const vk_shader_module> modules,
    const asset::vk_shader_data& shader_data, std::span<const VkDescriptorSetLayout> layouts) :
    _device{ &device }
{
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    shader_stages.reserve(modules.size());
    for (const auto& sh_module : modules) {
        VkPipelineShaderStageCreateInfo shader_stage{};
        shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage.pNext = nullptr;
        shader_stage.flags = 0;
        shader_stage.stage = sh_module.type();
        shader_stage.module = sh_module.handle();
        shader_stage.pName = "main";
        shader_stage.pSpecializationInfo = nullptr;

        shader_stages.push_back(shader_stage);
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};   
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &shader_data.vertex_binding;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<u32_t>(shader_data.vertex_descriptors.size());
    vertex_input_info.pVertexAttributeDescriptions = shader_data.vertex_descriptors.data();
    
    VkPipelineInputAssemblyStateCreateInfo assembly_info{};
    assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewport_info{};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.pViewports = &viewport;
    viewport_info.scissorCount = 1;
    viewport_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo raster_info{};
    raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_info.depthClampEnable = VK_FALSE;
    raster_info.rasterizerDiscardEnable = VK_FALSE;
    raster_info.polygonMode = VK_POLYGON_MODE_FILL;
    raster_info.lineWidth = 1.0f;
    raster_info.cullMode = VK_CULL_MODE_BACK_BIT;
    raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster_info.depthBiasEnable = VK_FALSE;
    raster_info.depthBiasConstantFactor = 0.0f;
    raster_info.depthBiasClamp = 0.0f;
    raster_info.depthBiasSlopeFactor = 0.0f;

    const auto num_samples = pass.raster_sample_count();
    VkPipelineMultisampleStateCreateInfo multisample_info{};
    multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_info.rasterizationSamples = num_samples;
    multisample_info.sampleShadingEnable = num_samples != VK_SAMPLE_COUNT_1_BIT;
    multisample_info.minSampleShading = 0.2f; // assume this value
    multisample_info.alphaToCoverageEnable = VK_FALSE;
    multisample_info.alphaToOneEnable = VK_FALSE;

    const auto depth_enabled = pass.depth_enabled();
    VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
    depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_info.depthTestEnable = depth_enabled;
    depth_stencil_info.depthWriteEnable = depth_enabled;
    depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_info.depthBoundsTestEnable = VK_FALSE; // assume no bounds
    depth_stencil_info.minDepthBounds = 0.0f;
    depth_stencil_info.maxDepthBounds = 1.0f;
    depth_stencil_info.stencilTestEnable = VK_FALSE; // and no stencil

    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    blend_attachment.blendEnable = depth_enabled;
    blend_attachment.srcColorBlendFactor = depth_enabled ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_ONE;
    blend_attachment.dstColorBlendFactor = depth_enabled ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ZERO;
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blend_info{};
    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_info.logicOpEnable = VK_FALSE;
    blend_info.logicOp = VK_LOGIC_OP_COPY;
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = &blend_attachment;
    blend_info.blendConstants[0] = 0.0f;
    blend_info.blendConstants[1] = 0.0f;
    blend_info.blendConstants[2] = 0.0f;
    blend_info.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = static_cast<u32_t>(layouts.size());
    pipeline_layout_info.pSetLayouts = layouts.data();
    pipeline_layout_info.pushConstantRangeCount = 0;
    vkCreatePipelineLayout(_device->handle(), &pipeline_layout_info, null_alloc, &_pipeline_layout);

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = static_cast<u32_t>(shader_stages.size());
    pipeline_info.pStages = shader_stages.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &assembly_info;
    pipeline_info.pTessellationState = nullptr;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &raster_info;
    pipeline_info.pMultisampleState = &multisample_info;
    pipeline_info.pDepthStencilState = &depth_stencil_info;
    pipeline_info.pColorBlendState = &blend_info;
    pipeline_info.pDynamicState = nullptr;
    pipeline_info.layout = _pipeline_layout;
    pipeline_info.renderPass = pass.handle();
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // assume no recreation
    pipeline_info.basePipelineIndex = -1;

    vkCreateGraphicsPipelines(_device->handle(), VK_NULL_HANDLE, 1, &pipeline_info, null_alloc, &_pipeline);
}

vk_pipeline_graphics::~vk_pipeline_graphics() {
    if (_device != nullptr) {
        vkDestroyPipeline(_device->handle(), _pipeline, null_alloc);
        vkDestroyPipelineLayout(_device->handle(), _pipeline_layout, null_alloc);
    }
}

void vk_pipeline_graphics::bind_pipeline(VkCommandBuffer buf) const {
    vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
}

vk_pipeline_graphics& vk_pipeline_graphics::operator=(vk_pipeline_graphics&& oth) {
    // destroy
    if (_device != nullptr) {
        vkDestroyPipeline(_device->handle(), _pipeline, null_alloc);
        vkDestroyPipelineLayout(_device->handle(), _pipeline_layout, null_alloc);
    }
    // move
    _device = oth._device;
    _pipeline = oth._pipeline;
    _pipeline_layout = oth._pipeline_layout;
    // null
    oth._device = nullptr;
    return *this;
}

}
