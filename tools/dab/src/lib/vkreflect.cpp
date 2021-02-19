#include <memory>

#include <spirv_cross/spirv_cross.hpp>

#include "dab/import.hpp"

namespace dab {

namespace spvc = spirv_cross;

namespace vkspv {

static uint32_t spir_typesize(const spvc::SPIRType& type) noexcept {
    // NOTE : breaks on double precisions, otherwise fine
    return 4 * type.vecsize * type.columns;
}

static VkFormat spir_vkformat(const spvc::SPIRType& type) noexcept {
    // TODO : only floats for now and vectors
    if (type.basetype == spvc::SPIRType::Float) {
        if (type.columns == 1) {
            return static_cast<VkFormat>(VK_FORMAT_R32_SFLOAT + (type.vecsize - 1) * 3);
        }
    }
    return VK_FORMAT_UNDEFINED;
}

static void write_ubos(
    const spvc::Compiler& compiler,
    const spvc::SmallVector<spvc::Resource>& resources,
    VkShaderStageFlagBits stage,
    std::vector<shader_vk_data::descriptor_info<VkDescriptorBufferInfo>>& out_ubo,
    std::vector<VkDescriptorSetLayoutBinding>& out_layout)
{
    out_ubo.reserve(out_ubo.size() + resources.size());
    out_layout.reserve(out_layout.size() + resources.size());

    for (const auto& resource : resources) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1; // NOTE : 1 descriptor
        binding.stageFlags = stage;

        VkDescriptorBufferInfo buffer{};
        buffer.buffer = VK_NULL_HANDLE;
        buffer.offset = 0; // NOTE : offset 0
        buffer.range = compiler.get_declared_struct_size(compiler.get_type(resource.base_type_id));

        out_ubo.emplace_back(out_layout.size(), buffer);
        out_layout.push_back(binding);
    }
}

static void write_combined_sampler(
    const spvc::Compiler& compiler,
    const spvc::SmallVector<spvc::Resource>& resources,
    VkShaderStageFlagBits stage,
    std::vector<shader_vk_data::descriptor_info<VkDescriptorImageInfo>>& out_combined,
    std::vector<VkDescriptorSetLayoutBinding>& out_layout)
{
    out_combined.reserve(out_combined.size() + resources.size());
    out_layout.reserve(out_layout.size() + resources.size());

    for (const auto& resource : resources) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1; // NOTE : 1 descriptor
        binding.stageFlags = stage;

        VkDescriptorImageInfo combined_sampler{};
        combined_sampler.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        combined_sampler.imageView = VK_NULL_HANDLE;
        combined_sampler.sampler = VK_NULL_HANDLE;

        out_combined.emplace_back(out_layout.size(), combined_sampler);
        out_layout.push_back(binding);
    }
}

}

shader_vk_data shader::vk_data() const {
    shader_vk_data vk_data;
    // these are too big for a stack, moving to heap
    std::unique_ptr compiler = std::make_unique<const spvc::Compiler>(modules[0].module_data.data(), modules[0].module_data.size());
    std::unique_ptr resources = std::make_unique<const spvc::ShaderResources>(compiler->get_shader_resources());

    // write input
    uint32_t input_stride = 0;
    vk_data.vertex_descriptors.reserve(resources->stage_inputs.size());
    for (const auto& input : resources->stage_inputs) {
        const auto& type = compiler->get_type(input.base_type_id);

        VkVertexInputAttributeDescription desc{};
        desc.location = compiler->get_decoration(input.id, spv::DecorationLocation);
        desc.binding = 0; // NOTE : binding to 0, everywhere
        desc.format = vkspv::spir_vkformat(type);
        desc.offset = input_stride;

        input_stride += vkspv::spir_typesize(type);
        vk_data.vertex_descriptors.push_back(desc);
    }

    vk_data.vertex_binding.binding = 0; // NOTE : binding to 0
    vk_data.vertex_binding.stride = input_stride;
    vk_data.vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // NOTE : not instance

    // write input uniforms
    vkspv::write_ubos(*compiler, resources->uniform_buffers, modules[0].stage,
        vk_data.buffer_infos, vk_data.layout_bindings);
    vkspv::write_combined_sampler(*compiler, resources->sampled_images, modules[0].stage,
        vk_data.comb_sampler_infos, vk_data.layout_bindings);

    // write other stages
    for (auto i = 1u; i < modules.size(); ++i) {
        compiler = std::make_unique<const spvc::Compiler>(modules[i].module_data.data(), modules[i].module_data.size());
        resources = std::make_unique<const spvc::ShaderResources>(compiler->get_shader_resources());

        vkspv::write_ubos(*compiler, resources->uniform_buffers, modules[i].stage,
            vk_data.buffer_infos, vk_data.layout_bindings);
        vkspv::write_combined_sampler(*compiler, resources->sampled_images, modules[i].stage,
            vk_data.comb_sampler_infos, vk_data.layout_bindings);
    }
    return vk_data;
}

}