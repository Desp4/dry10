#include "pipeline_resources.hpp"

#include <numeric>
#include <algorithm>

#include "vk_initers.hpp"

namespace dry {

template<VkDescriptorType Desc_T, uint32_t Set = pipeline_resources::resource_binding_point>
auto extract_desc_bindings(std::vector<pipeline_resources::shader_layout_info>& in) -> std::remove_reference_t<decltype(in)> {
    return extract_shader_layouts<[](pipeline_resources::shader_layout_info& binding){ return binding.type == Desc_T && binding.set == Set; }>(in);
}

pipeline_resources::pipeline_resources(const vkw::vk_device& device, u32_t frame_count, std::vector<shader_layout_info> layout_bindings) :
    _device{ &device },
    _frame_count{ frame_count },
    _material_data_stride{ 0 }
{
    const bool has_resources = layout_bindings.size() != 0 ||
        std::count_if(layout_bindings.begin(), layout_bindings.end(), [](auto& el) { return el.set == resource_binding_point;}) != 0;
    if (!has_resources) {
        return;
    }

    assure_material_ssbo(layout_bindings);

    // create layout
    std::vector<VkDescriptorSetLayoutBinding> shader_bindings;
    shader_bindings.reserve(layout_bindings.size());

    for (const auto& binding : layout_bindings) {
        shader_bindings.push_back(layout_binding_from_reflect_info(binding));
    }
    
    _layout = vkw::vk_descriptor_layout{ device, shader_bindings };

    assure_input input_ctx;
    input_ctx.frame_descriptors.resize(frame_count);

    create_ubos(input_ctx, extract_desc_bindings<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(layout_bindings));
    create_ssbos(input_ctx, extract_desc_bindings<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>(layout_bindings));

    _desc_pool = vkw::vk_descriptor_pool{ device, input_ctx.pool_sizes, frame_count };
    _descriptors.resize(frame_count);

    _desc_pool.create_sets(_descriptors, _layout.handle());
    for (auto i = 0u; i < input_ctx.frame_descriptors.size(); ++i) {
        auto& frame_desc_writes = input_ctx.frame_descriptors[i];
        for (auto& desc_write : frame_desc_writes) {
            desc_write.dstSet = _descriptors[i];
        }
        vkUpdateDescriptorSets(device.handle(), static_cast<u32_t>(frame_desc_writes.size()), frame_desc_writes.data(), 0, nullptr);
    }
}


pipeline_resources::~pipeline_resources() {
    for (const auto& ubo : _ubos) {
        ubo.unmap();
    }
    for (const auto& ssbo : _ssbos) {
        ssbo.unmap();
    }
}

void pipeline_resources::bind_resources(u32_t frame, VkCommandBuffer cmd_buffer, VkPipelineLayout layout) const {
    if (_descriptors.size() != 0) {
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, resource_binding_point, 1, &_descriptors[frame], 0, nullptr);
    }
}

void pipeline_resources::assure_material_ssbo(std::span<const shader_layout_info> bindings) {
    const auto it = std::find_if(bindings.begin(), bindings.end(),
        [](const auto& binding) { return binding.set == resource_binding_point && binding.binding == material_ssbo_location; }
    );

    if (it == bindings.end()) {
        _material_data_stride = 0;
        return;
    }

    if (it->type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
        LOG_ERR("set 2 binding 0 is reserved for material ssbo, invalid shader");
        dbg::panic();
    }

    _material_data_stride = it->stride;
}

void pipeline_resources::assure_buffer_location_map_size(std::span<const shader_layout_info> bindings) {
    auto max_binding_lambda = [](const shader_layout_info& l, const shader_layout_info& r) {
        return l.binding < r.binding;
    };

    const u32_t max_location = _frame_count * (1 + std::max_element(bindings.begin(), bindings.end(), max_binding_lambda)->binding);
    if (max_location > _buffer_location_map.size()) {
        _buffer_location_map.resize(max_location);
    }
}

void pipeline_resources::create_ubos(assure_input& input_ctx, std::span<const shader_layout_info> bindings) {
    if (bindings.size() == 0) {
        return;
    }

    u32_t combined_size = 0;    
    for (const auto& binding : bindings) {
        combined_size += binding.stride * binding.count;
    }

    std::vector<std::byte*> mapped_ptrs;
    mapped_ptrs.reserve(_frame_count);
    _ubos.reserve(_frame_count);

    for (auto& frame_descs : input_ctx.frame_descriptors) {
        _ubos.emplace_back(*_device, combined_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        mapped_ptrs.emplace_back(_ubos.back().map<std::byte>());
    }

    assure_buffer_location_map_size(bindings);
    auto buffer_infos = std::make_unique<VkDescriptorBufferInfo[]>(_frame_count * bindings.size());
    u32_t it = 0;    
    u32_t offset = 0;

    for (const auto& binding : bindings) {
        VkWriteDescriptorSet desc_write = desc_write_from_binding(layout_binding_from_reflect_info(binding));
        const u32_t buffer_range = binding.stride * binding.count;

        for (auto i = 0u; i < _frame_count; ++i) {
            buffer_infos[it].buffer = _ubos[i].handle();
            buffer_infos[it].range = buffer_range;
            buffer_infos[it].offset = offset;

            desc_write.pBufferInfo = &buffer_infos[it];
            input_ctx.frame_descriptors[i].push_back(desc_write);

            auto& buffer_loc = _buffer_location_map[binding.binding * _frame_count + i];
            buffer_loc.dst = mapped_ptrs[i];
            buffer_loc.offset = offset;

            ++it;
        }
        offset += buffer_range;
    }

    input_ctx.resources.push_back(std::move(buffer_infos));
    input_ctx.pool_sizes.emplace_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<u32_t>(bindings.size()));
}

void pipeline_resources::create_ssbos(assure_input& input_ctx, std::span<const shader_layout_info> bindings) {
    if (bindings.size() == 0) {
        return;
    }
    // TODO : buffer is fixed size
    constexpr u64_t ssbo_element_count = 8192;

    _ssbos.reserve(bindings.size() * _frame_count);
    auto buffer_infos = std::make_unique<VkDescriptorBufferInfo[]>(_frame_count * bindings.size());

    assure_buffer_location_map_size(bindings);
    u32_t it = 0;

    for (const auto& binding : bindings) {
        VkWriteDescriptorSet desc_write = desc_write_from_binding(layout_binding_from_reflect_info(binding));
        const u32_t buffer_range = binding.stride * binding.count * ssbo_element_count;

        for (auto i = 0u; i < _frame_count; ++i) {
            _ssbos.emplace_back(*_device, buffer_range, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            buffer_infos[it].buffer = _ssbos.back().handle();
            buffer_infos[it].range = buffer_range;
            buffer_infos[it].offset = 0;

            desc_write.pBufferInfo = &buffer_infos[it];
            input_ctx.frame_descriptors[i].push_back(desc_write);

            auto& buffer_loc = _buffer_location_map[binding.binding * _frame_count + i];
            buffer_loc.dst = _ssbos.back().map<std::byte>();
            buffer_loc.offset = 0;

            ++it;
        }
    }

    input_ctx.resources.push_back(std::move(buffer_infos));
    input_ctx.pool_sizes.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<u32_t>(_ssbos.size()));
}

}