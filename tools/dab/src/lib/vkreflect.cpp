#include "dab/import.hpp"

#include <spirv_cross/spirv_cross.hpp>

namespace dab
{
    namespace spvc = spirv_cross;

    namespace vkspv
    {
        static uint32_t typeSize(const spvc::SPIRType& type)
        {
            // NOTE : breaks on double precisions, otherwise fine
            return 4 * type.vecsize * type.columns;
        }

        static VkFormat vkFormat(const spvc::SPIRType& type)
        {
            // TODO : only floats for now and vectors
            if (type.basetype == spvc::SPIRType::Float)
            {
                if (type.columns == 1)
                    return static_cast<VkFormat>(VK_FORMAT_R32_SFLOAT + (type.vecsize - 1) * 3);
            }
            return VK_FORMAT_UNDEFINED;
        }

        static void writeUBOs(
            const spvc::Compiler& compiler,
            const spvc::SmallVector<spvc::Resource>& resources,
            VkShaderStageFlagBits stage,
            std::vector<ShaderVkData::DescriptorInfo<VkDescriptorBufferInfo>>& uboOut,
            std::vector<VkDescriptorSetLayoutBinding>& layoutOut)
        {
            uboOut.reserve(uboOut.size() + resources.size());
            layoutOut.reserve(layoutOut.size() + resources.size());

            for (const auto& resource : resources)
            {
                VkDescriptorSetLayoutBinding binding{};
                binding.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                binding.descriptorCount = 1; // NOTE : 1 descriptor
                binding.stageFlags = stage;

                VkDescriptorBufferInfo buffer{};
                buffer.buffer = VK_NULL_HANDLE;
                buffer.offset = 0; // NOTE : offset 0
                buffer.range = compiler.get_declared_struct_size(compiler.get_type(resource.base_type_id));

                uboOut.emplace_back(layoutOut.size(), buffer);
                layoutOut.push_back(binding);
            }
        }

        static void writeCombinedSampler(
            const spvc::Compiler& compiler,
            const spvc::SmallVector<spvc::Resource>& resources,
            VkShaderStageFlagBits stage,
            std::vector<ShaderVkData::DescriptorInfo<VkDescriptorImageInfo>>& combinedOut,
            std::vector<VkDescriptorSetLayoutBinding>& layoutOut)
        {
            combinedOut.reserve(combinedOut.size() + resources.size());
            layoutOut.reserve(layoutOut.size() + resources.size());

            for (const auto& resource : resources)
            {
                VkDescriptorSetLayoutBinding binding{};
                binding.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                binding.descriptorCount = 1; // NOTE : 1 descriptor
                binding.stageFlags = stage;

                VkDescriptorImageInfo combImage{};
                combImage.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                combImage.imageView = VK_NULL_HANDLE;
                combImage.sampler = VK_NULL_HANDLE;

                combinedOut.emplace_back(layoutOut.size(), combImage);
                layoutOut.push_back(binding);
            }
        }
    }

    ShaderVkData Shader::vkData() const
    {
        ShaderVkData vkData;

        const spvc::Compiler inputComp(modules[0].data.data(), modules[0].data.size());
        const spvc::ShaderResources inputRes = inputComp.get_shader_resources();

        // write input
        uint32_t inputStride = 0;
        vkData.vertexDescriptors.reserve(inputRes.stage_inputs.size());
        for (const auto& input : inputRes.stage_inputs)
        {
            const auto& type = inputComp.get_type(input.base_type_id);

            VkVertexInputAttributeDescription desc{};
            desc.location = inputComp.get_decoration(input.id, spv::DecorationLocation);
            desc.binding = 0; // NOTE : binding to 0, everywhere
            desc.format = vkspv::vkFormat(type);
            desc.offset = inputStride;

            inputStride += vkspv::typeSize(type);
            vkData.vertexDescriptors.push_back(desc);
        }
            
        vkData.vertexBinding.binding = 0; // NOTE : binding to 0
        vkData.vertexBinding.stride = inputStride;
        vkData.vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // NOTE : not instance

        // write input uniforms
        vkspv::writeUBOs(inputComp, inputRes.uniform_buffers, modules[0].stage,
            vkData.bufferInfos, vkData.layoutBindings);
        vkspv::writeCombinedSampler(inputComp, inputRes.sampled_images, modules[0].stage,
            vkData.combImageInfos, vkData.layoutBindings);

        // write other stages
        for (int i = 1; i < modules.size(); ++i)
        {
            const spvc::Compiler compiler(modules[i].data.data(), modules[i].data.size());
            const spvc::ShaderResources resources = compiler.get_shader_resources();

            vkspv::writeUBOs(compiler, resources.uniform_buffers, modules[i].stage,
                vkData.bufferInfos, vkData.layoutBindings);
            vkspv::writeCombinedSampler(compiler, resources.sampled_images, modules[i].stage,
                vkData.combImageInfos, vkData.layoutBindings);
        }

        return vkData;
    }
}