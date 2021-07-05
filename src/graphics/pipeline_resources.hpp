#pragma once

#ifndef DRY_GR_PIPELINE_RESOURCES_H
#define DRY_GR_PIPELINE_RESOURCES_H

#include <variant>
#include <memory>

#include "asset/vk_reflect.hpp"

#include "vkw/buffer.hpp"
#include "vkw/desc/desclayout.hpp"
#include "vkw/desc/descpool.hpp"

namespace dry {

class pipeline_resources {
public:
    using shader_layout_info = asset::vk_shader_data::layout_binding_info;

    static constexpr u32_t resource_binding_point = 2;
    static constexpr u32_t material_ssbo_location = 0;

    pipeline_resources() = default;
    pipeline_resources(pipeline_resources&&) = default;
    pipeline_resources(const vkw::vk_device& device, u32_t frame_count, std::vector<shader_layout_info> layout_bindings);
    ~pipeline_resources();

    template<typename T>
    void write_to_buffer(u32_t frame, u32_t binding, const T& value);
    template<typename T>
    void write_to_buffer(u32_t frame, u32_t binding, const std::vector<T>& values);
    template<typename T>
    void write_to_buffer(u32_t frame, u32_t binding, std::span<const T> values);

    template<typename T>
    T* buffer_data(u32_t frame, u32_t binding);

    void bind_resources(u32_t frame, VkCommandBuffer cmd_buffer, VkPipelineLayout layout) const;

    u32_t material_stride() const { return _material_data_stride; }
    VkDescriptorSetLayout descriptor_layout() const { return _layout.handle(); }

    bool has_materials() const { return _material_data_stride != 0; }
    bool has_resources() const { return _layout.handle() != VK_NULL_HANDLE; }

    pipeline_resources& operator=(pipeline_resources&&) = default;

private:
    using desc_write_res = std::variant<std::unique_ptr<VkDescriptorBufferInfo[]>, std::unique_ptr<VkDescriptorImageInfo[]>>;
    struct assure_input {
        std::vector<desc_write_res> resources;
        std::vector<std::vector<VkWriteDescriptorSet>> frame_descriptors;
        std::vector<VkDescriptorPoolSize> pool_sizes;
    };
    struct buffer_lookup {
        std::byte* dst;
        u32_t offset;
    };

    void assure_buffer_location_map_size(std::span<const shader_layout_info> bindings);
    void assure_material_ssbo(std::span<const shader_layout_info> bindings);

    void create_ubos(assure_input& input_ctx, std::span<const shader_layout_info> bindings);
    void create_ssbos(assure_input& input_ctx, std::span<const shader_layout_info> bindings);

    const vkw::vk_device* _device;

    vkw::vk_descriptor_layout _layout;
    vkw::vk_descriptor_pool _desc_pool;
    std::vector<VkDescriptorSet> _descriptors;

    /* NOTE : buffers are DEVICE_LOCAL */
    // frame_count in size, each buffer has a ubo for each frame
    std::vector<vkw::vk_buffer> _ubos;
    // ssbo_count * frame_count in size, layout: {ssbo0_frame0, ssbo0_frame1 ... ssbo1_frame0 ...}
    std::vector<vkw::vk_buffer> _ssbos;
    // same layout as for ssbos
    std::vector<buffer_lookup> _buffer_location_map;

    u32_t _frame_count;
    u32_t _material_data_stride;
};



template<typename T>
void pipeline_resources::write_to_buffer(u32_t frame, u32_t binding, const T& value) {
    write_to_buffer(frame, binding, std::span{ &value, 1 });
}
template<typename T>
void pipeline_resources::write_to_buffer(u32_t frame, u32_t binding, const std::vector<T>& values) {
    write_to_buffer(frame, binding, std::span{ values.begin(), values.end() });
}
template<typename T>
void pipeline_resources::write_to_buffer(u32_t frame, u32_t binding, std::span<const T> values) {
    std::copy(values.begin(), values.end(), buffer_data<T>(frame, binding));
}

template<typename T>
T* pipeline_resources::buffer_data(u32_t frame, u32_t binding) {
    const auto& dst_info = _buffer_location_map[binding * _frame_count + frame];
    return reinterpret_cast<T*>(dst_info.dst + dst_info.offset);
}

}

#endif