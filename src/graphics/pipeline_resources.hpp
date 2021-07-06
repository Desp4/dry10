#pragma once

#ifndef DRY_GR_PIPELINE_RESOURCES_H
#define DRY_GR_PIPELINE_RESOURCES_H

#include <variant>
#include <memory>

#include "asset/vk_reflect.hpp"

#include "vkw/cmd/cmdbuffer.hpp"
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

    template<typename T = std::byte>
    T* ssbo_data(u32_t frame, u32_t binding);
    template<typename T = std::byte>
    T* ubo_data(u32_t binding);

    void transfer_staging_ubos(const vkw::vk_cmd_buffer& cmd, u32_t frame);

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

    // frame_count in size, each buffer has ubos for each frame
    std::vector<vkw::vk_buffer> _ubos;
    // ssbo_count * frame_count in size, layout: {ssbo0_frame0, ssbo0_frame1 ... ssbo1_frame0 ...}
    std::vector<vkw::vk_buffer> _ssbos;
    // same layout as for ssbos
    std::vector<std::byte*> _ssbo_location_map;
    // points to staging buffer, same size as number of ubos
    std::vector<u32_t> _ubo_location_map;

    struct {
        vkw::vk_buffer buffer;
        std::byte* data = nullptr;
    } _staging_ubo;
    

    u32_t _frame_count;
    u32_t _material_data_stride;
};

template<typename T>
T* pipeline_resources::ssbo_data(u32_t frame, u32_t binding) {
    return reinterpret_cast<T*>(_ssbo_location_map[binding * _frame_count + frame]);
}
template<typename T>
T* pipeline_resources::ubo_data(u32_t binding) {
    return reinterpret_cast<T*>(_staging_ubo.data + _ubo_location_map[binding]);
}


}

#endif