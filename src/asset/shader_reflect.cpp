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
    // TODO : only floats for now and vectors
    if (type.basetype == spvc::SPIRType::Float) {
        if (type.columns == 1) {
            return static_cast<VkFormat>(VK_FORMAT_R32_SFLOAT + (type.vecsize - 1) * 3);
        }
    }
    return VK_FORMAT_UNDEFINED;
}

template<VkDescriptorType Desc_Type>
static void write_buffers(
    const spvc::Compiler& compiler,
    const spvc::SmallVector<spvc::Resource>& resources,
    VkShaderStageFlagBits stage,
    std::vector<vk_shader_data::descriptor_info<VkDescriptorBufferInfo>>& out_ubo,
    std::vector<VkDescriptorSetLayoutBinding>& out_layout, std::vector<VkDescriptorSetLayoutBinding>& exclude)
{
    out_ubo.reserve(out_ubo.size() + resources.size());
    out_layout.reserve(out_layout.size() + resources.size());

    for (const auto& resource : resources) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        binding.descriptorType = Desc_Type;
        binding.descriptorCount = 1; // NOTE : 1 descriptor
        binding.stageFlags = stage;
        binding.pImmutableSamplers = nullptr;
        
        auto it = std::find(exclude.begin(), exclude.end(), binding);
        if (it != exclude.end()) {
            exclude.erase(it);
            continue;
        }

        VkDescriptorBufferInfo buffer{};
        buffer.buffer = VK_NULL_HANDLE;
        buffer.offset = 0; // NOTE : offset 0
        buffer.range = compiler.get_declared_struct_size(compiler.get_type(resource.base_type_id));

        out_ubo.emplace_back(static_cast<u32_t>(out_layout.size()), buffer);
        out_layout.push_back(binding);
    }
}

static void write_combined_sampler(
    const spvc::Compiler& compiler,
    const spvc::SmallVector<spvc::Resource>& resources,
    VkShaderStageFlagBits stage,
    std::vector<vk_shader_data::descriptor_info<VkDescriptorImageInfo>>& out_combined,
    std::vector<VkDescriptorSetLayoutBinding>& out_layout, std::vector<VkDescriptorSetLayoutBinding>& exclude)
{
    out_combined.reserve(out_combined.size() + resources.size());
    out_layout.reserve(out_layout.size() + resources.size());

    for (const auto& resource : resources) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1; // NOTE : 1 descriptor
        binding.stageFlags = stage;
        binding.pImmutableSamplers = nullptr;

        auto it = std::find(exclude.begin(), exclude.end(), binding);
        if (it != exclude.end()) {
            exclude.erase(it);
            continue;
        }

        // TODO : ?? I don't even fill these
        VkDescriptorImageInfo combined_sampler{};
        combined_sampler.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        combined_sampler.imageView = VK_NULL_HANDLE;
        combined_sampler.sampler = VK_NULL_HANDLE;

        out_combined.emplace_back(static_cast<u32_t>(out_layout.size()), combined_sampler);
        out_layout.push_back(binding);
    }
}

}

vk_shader_data shader_vk_info(const shader_source& shader, std::vector<VkDescriptorSetLayoutBinding> exclude) {
    vk_shader_data vk_data;
    // these are too big for a stack, moving to heap
    std::unique_ptr compiler = std::make_unique<const spvc::Compiler>(
        reinterpret_cast<const u32_t*>(shader.vert_stage.spirv.data()), shader.vert_stage.spirv.size() / sizeof(u32_t));
    std::unique_ptr resources = std::make_unique<const spvc::ShaderResources>(compiler->get_shader_resources());

    // write input
    u32_t input_stride = 0;
    vk_data.vertex_descriptors.reserve(resources->stage_inputs.size());
    // NOTE : need to sort to get reliable offsets
    std::vector<spvc::Resource> stage_inputs;
    stage_inputs.reserve(resources->stage_inputs.size());
    auto sort_vert_location_lambda = [&compiler](const spvc::Resource& el, const spvc::Resource& val) {
        return compiler->get_decoration(el.id, spv::DecorationLocation) < compiler->get_decoration(val.id, spv::DecorationLocation);
    };

    for (const auto& input : resources->stage_inputs) {
        stage_inputs.insert(
            std::lower_bound(stage_inputs.begin(), stage_inputs.end(), input, sort_vert_location_lambda),
            input
        );
    }

    for (const auto& input : stage_inputs) {
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
    const auto vert_stage = shader_vk_stage(shader.vert_stage.stage);

    vkspv::write_buffers<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(
        *compiler, resources->uniform_buffers, vert_stage,
        vk_data.buffer_infos, vk_data.layout_bindings, exclude
    );
    vkspv::write_buffers<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>(
        *compiler, resources->storage_buffers, vert_stage,
        vk_data.buffer_infos, vk_data.layout_bindings, exclude
    );
    vkspv::write_combined_sampler(
        *compiler, resources->sampled_images, vert_stage,
        vk_data.comb_sampler_infos, vk_data.layout_bindings, exclude
    );

    // write other stages
    for (const auto& stage : shader.oth_stages) {
        compiler = std::make_unique<const spvc::Compiler>(reinterpret_cast<const u32_t*>(stage.spirv.data()), stage.spirv.size() / sizeof(u32_t));
        resources = std::make_unique<const spvc::ShaderResources>(compiler->get_shader_resources());

        const auto vk_stage = shader_vk_stage(stage.stage);

        vkspv::write_buffers<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(
            *compiler, resources->uniform_buffers, vert_stage,
            vk_data.buffer_infos, vk_data.layout_bindings, exclude
        );
        vkspv::write_buffers<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>(
            *compiler, resources->storage_buffers, vert_stage,
            vk_data.buffer_infos, vk_data.layout_bindings, exclude
        );
        vkspv::write_combined_sampler(
            *compiler, resources->sampled_images, vk_stage,
            vk_data.comb_sampler_infos, vk_data.layout_bindings, exclude
        );
    }
    if (exclude.size() != 0) {
        LOG_ERR("Exclude layout bindings not exhausted, invalid shader");
        dbg::panic();
    }

    return vk_data;
}

}
