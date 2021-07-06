#pragma once

#ifndef DRY_GR_VK_INITERS_H
#define DRY_GR_VK_INITERS_H

#include "asset/vk_reflect.hpp"

#include "vkw/queue/queue.hpp"
#include "vkw/image/imageviewpair.hpp"

namespace dry {

struct vk_vertex_input {
    // only 1
    VkVertexInputBindingDescription binding_desc;
    std::vector<VkVertexInputAttributeDescription> attribute_desc;
};

vk_vertex_input vertex_bindings_to_input(std::vector<asset::vk_shader_data::vertex_binding_info> bindings);

vkw::vk_image_view_pair create_sampled_texture(const vkw::vk_device& device,
    const vkw::vk_queue& g_queue, const vkw::vk_queue& t_queue, const_byte_span data,
    VkExtent2D dimensions, u32_t mip_lvls, VkFormat img_format
);

constexpr VkWriteDescriptorSet desc_write_from_binding(VkDescriptorSetLayoutBinding binding) {
    VkWriteDescriptorSet desc_write{};

    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstBinding = binding.binding;
    desc_write.descriptorType = binding.descriptorType;
    desc_write.descriptorCount = binding.descriptorCount;
    return desc_write;
}

constexpr VkBufferUsageFlags descriptor_type_to_buffer_usage(VkDescriptorType type) {
    switch (type) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    default: LOG_ERR("Can't initialize a buffer from this descriptor"); dbg::panic(); // TODO : fmt type into log err
    }
}

constexpr VkDescriptorSetLayoutBinding layout_binding_from_reflect_info(const asset::vk_shader_data::layout_binding_info& binding_info) {
    VkDescriptorSetLayoutBinding ret_layout_binding{};
    ret_layout_binding.binding = binding_info.binding;
    ret_layout_binding.descriptorCount = binding_info.count;
    ret_layout_binding.descriptorType = binding_info.type;
    ret_layout_binding.stageFlags = binding_info.stage;
    return ret_layout_binding;
}

template<auto Func>
auto extract_shader_layouts(std::vector<asset::vk_shader_data::layout_binding_info>& in) -> std::remove_reference_t<decltype(in)> {
    using desc_vec_t = std::remove_reference_t<decltype(in)>;

    desc_vec_t ret_arr;
    desc_vec_t::iterator it = in.begin();
    while (it != in.end()) {
        it = std::find_if(it, in.end(), Func);
        if (it == in.end()) {
            break;
        }

        ret_arr.push_back(std::move(*it));
        it = in.erase(it);
    }

    return ret_arr;
}

}

#endif