#include "vk_reflect.hpp"

#include <memory>

#include <spirv_cross.hpp>

static constexpr bool operator==(const VkDescriptorSetLayoutBinding& l, const VkDescriptorSetLayoutBinding& r) noexcept {
    return
        l.binding == r.binding && l.descriptorCount == r.descriptorCount &&
        l.descriptorType == r.descriptorType && l.pImmutableSamplers == r.pImmutableSamplers &&
        l.stageFlags == r.stageFlags;
}

namespace dry::asset {

namespace spvc = spirv_cross;

namespace vkspv {

static u32_t spir_typesize(const spvc::SPIRType& type) noexcept {
    // NOTE : breaks on double precisions, otherwise fine
    return 4 * type.vecsize * type.columns;
}

static VkFormat spir_vkformat(const spvc::SPIRType& type) noexcept {
    
    auto vk_format_offset = [](auto base, const auto& type) {
        // abuse VkFormat enum order, look into the vulkan header if have questions
        return static_cast<VkFormat>(base + (type.vecsize - 1) * 3);
    };

    if (type.columns != 1) {
        LOG_ERR("Matrix input not supported by vulkan, split it into vectors please");
        return VK_FORMAT_UNDEFINED;
    }

    switch (type.basetype) {
    case spvc::SPIRType::BaseType::Float: return vk_format_offset(VK_FORMAT_R32_SFLOAT, type);
    case spvc::SPIRType::BaseType::Int:   return vk_format_offset(VK_FORMAT_R32_SINT, type);
    case spvc::SPIRType::BaseType::UInt:  return vk_format_offset(VK_FORMAT_R32_UINT, type);
    default: return VK_FORMAT_UNDEFINED;
    }
}

template<VkDescriptorType Desc_Type>
static void write_layout_bindings(
    const spvc::Compiler& compiler,
    const spvc::SmallVector<spvc::Resource>& resources,
    VkShaderStageFlagBits stage,
    std::vector<vk_shader_data::layout_binding_info>& out_bindings)
{
    out_bindings.reserve(out_bindings.size() + resources.size());

    for (const auto& resource : resources) {
        const auto& type = compiler.get_type(resource.type_id);

        vk_shader_data::layout_binding_info binding{};
        binding.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        binding.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        binding.type = Desc_Type;
        binding.count = type.array.size() != 0 ? type.array[0] : 1;
        binding.stage = stage;
        binding.stride = type.member_types.empty() ? 0 : compiler.get_declared_struct_size(type);

        out_bindings.push_back(binding);
    }
}

}

vk_shader_data shader_vk_info(const shader_source& shader) {
    vk_shader_data vk_data;
    // these are too big for a stack, moving to heap
    std::unique_ptr compiler = std::make_unique<const spvc::Compiler>(
        reinterpret_cast<const u32_t*>(shader.vert_stage.spirv.data()), shader.vert_stage.spirv.size() / sizeof(u32_t));
    std::unique_ptr resources = std::make_unique<const spvc::ShaderResources>(compiler->get_shader_resources());

    // write input
    vk_data.vertex_descriptors.reserve(resources->stage_inputs.size());
    for (const auto& input : resources->stage_inputs) {
        const auto& type = compiler->get_type(input.base_type_id);

        vk_shader_data::vertex_binding_info desc_info;
        desc_info.location = compiler->get_decoration(input.id, spv::DecorationLocation);
        desc_info.format = vkspv::spir_vkformat(type);
        desc_info.stride = vkspv::spir_typesize(type);

        vk_data.vertex_descriptors.push_back(desc_info);
    }

    // write input uniforms
    const auto vert_stage = shader_vk_stage(shader.vert_stage.stage);

    vkspv::write_layout_bindings<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(*compiler, resources->uniform_buffers, vert_stage,vk_data.layout_bindings);
    vkspv::write_layout_bindings<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>(*compiler, resources->storage_buffers, vert_stage, vk_data.layout_bindings);
    vkspv::write_layout_bindings<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(*compiler, resources->sampled_images, vert_stage, vk_data.layout_bindings);
    vkspv::write_layout_bindings<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>(*compiler, resources->separate_images, vert_stage, vk_data.layout_bindings);
    vkspv::write_layout_bindings<VK_DESCRIPTOR_TYPE_SAMPLER>(*compiler, resources->separate_samplers, vert_stage, vk_data.layout_bindings);

    // write other stages
    for (const auto& stage : shader.oth_stages) {
        compiler = std::make_unique<const spvc::Compiler>(reinterpret_cast<const u32_t*>(stage.spirv.data()), stage.spirv.size() / sizeof(u32_t));
        resources = std::make_unique<const spvc::ShaderResources>(compiler->get_shader_resources());

        const auto vk_stage = shader_vk_stage(stage.stage);

        vkspv::write_layout_bindings<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(*compiler, resources->uniform_buffers, vk_stage, vk_data.layout_bindings);
        vkspv::write_layout_bindings<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>(*compiler, resources->storage_buffers, vk_stage, vk_data.layout_bindings);
        vkspv::write_layout_bindings<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(*compiler, resources->sampled_images, vk_stage, vk_data.layout_bindings);
        vkspv::write_layout_bindings<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>(*compiler, resources->separate_images, vk_stage, vk_data.layout_bindings);
        vkspv::write_layout_bindings<VK_DESCRIPTOR_TYPE_SAMPLER>(*compiler, resources->separate_samplers, vk_stage, vk_data.layout_bindings);
    }

    return vk_data;
}

}
