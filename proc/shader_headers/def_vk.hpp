#pragma once

#include "pcsrtype.hpp"

namespace pcsr::def
{
    static constexpr auto createData()
    {
        VkVertexInputBindingDescription ibind{
            .binding   = 0,
            .stride    = 20,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };
        std::array<VkVertexInputAttributeDescription, 2> idesc{};
        idesc[0] = VkVertexInputAttributeDescription{
            .location = 0,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = 0 };
        idesc[1] = VkVertexInputAttributeDescription{
            .location = 1,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32_SFLOAT,
            .offset   = 12 };
        std::array<VkDescriptorSetLayoutBinding, 2> lbind{};
        lbind[0] = VkDescriptorSetLayoutBinding{
            .binding         = 0,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT };
        lbind[1] = VkDescriptorSetLayoutBinding{
            .binding         = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT };
        std::array<VkDescriptorBufferInfo, 1> ubuff{};
        ubuff[0] = VkDescriptorBufferInfo{
            .buffer   = VK_NULL_HANDLE,
            .offset   = 0,
            .range    = 192 };
        return ShaderVkData<2, 2, 1>{ .input{ ibind, idesc }, .layoutBindings{ lbind }, .bufferInfos{ ubuff } };
    }

    constexpr ShaderVkData defData = createData();
}
