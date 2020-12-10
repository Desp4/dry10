#pragma once

#include <array>
#include <span>

#include <vulkan/vulkan.h>

namespace pcsr
{
    struct ShaderData
    {
        struct
        {
            VkVertexInputBindingDescription binding;
            std::span<const VkVertexInputAttributeDescription> descriptors;
        } input;
        std::span<const VkDescriptorSetLayoutBinding> layoutBindings;
        std::span<const VkDescriptorBufferInfo> bufferInfos;
    };

    template<size_t DescN, size_t BindN, size_t BuffN>
    struct ShaderVkData
    {
        using DescArr = std::array<VkVertexInputAttributeDescription, DescN>;
        using BindArr = std::array<VkDescriptorSetLayoutBinding, BindN>;
        using BuffArr = std::array<VkDescriptorBufferInfo, BuffN>;

        struct
        {
            VkVertexInputBindingDescription binding;
            DescArr descriptors;
        } input;
        BindArr layoutBindings;
        BuffArr bufferInfos;

        constexpr operator ShaderData() const
        {
            return { { input.binding, input.descriptors }, layoutBindings, bufferInfos };
        }
    };
}