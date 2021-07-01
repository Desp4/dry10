#include "texarr.hpp"

#include "util/util.hpp"
#include "vk_initers.hpp"

namespace dry {

void texture_array::add_texture(u32_t index, VkImageView texture) {
    texture_array_infos[index].imageView = texture;
    for (auto i = 0u; i < texture_update_status.size(); ++i) {
        texture_update_status[i] = false;
    }
}

void texture_array::remove_texture(u32_t index) {
    add_texture(index, dummy_image.view().handle());
}
void texture_array::update_descriptors(const vkw::vk_device& device, u32_t frame) {
    if (texture_update_status[frame] != false) {
        return;
    }

    auto desc_write = desc_write_from_binding(layout_binding_from_reflect_info(texture_array::texarr_layout_binding));
    desc_write.pImageInfo = texture_array_infos.data();
    desc_write.dstSet = texarr_descriptors[frame];

    vkUpdateDescriptorSets(device.handle(), 1, &desc_write, 0, nullptr);

    texture_update_status[frame] = true;
}

texture_array create_texture_array(const vkw::vk_device& device, const vkw::vk_queue_graphics& graphics_queue, u32_t frame_count) {
    texture_array texarr;
    
    constexpr VkExtent2D dummy_extent{ 4, 4 };
    constexpr std::array layout_bindings = generate_array(texture_array::layout_bindings, layout_binding_from_reflect_info);

    auto desc_pool_lambda = [frame_count](decltype(layout_bindings)::value_type val) -> VkDescriptorPoolSize {
        return VkDescriptorPoolSize{ .type = val.descriptorType, .descriptorCount = frame_count };
    };

    texarr.texarr_descriptor_layout = vkw::vk_descriptor_layout{ device, layout_bindings };

    const std::array desc_pool_sizes = generate_array(layout_bindings, desc_pool_lambda);
    texarr.texarr_descriptor_pool = vkw::vk_descriptor_pool{ device, desc_pool_sizes, frame_count };

    texarr.sampler = vkw::vk_tex_sampler{ device, texture_array::sampler_mip_levels };
    texarr.dummy_image = vkw::vk_image_view_pair{
        device, dummy_extent, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_ASPECT_COLOR_BIT
    };
    graphics_queue.transition_image_layout(texarr.dummy_image.image(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    texarr.texture_update_status.resize(texture_array::array_size, true);
    texarr.texture_array_infos.resize(texture_array::array_size, VkDescriptorImageInfo{
        .sampler = VK_NULL_HANDLE, .imageView = texarr.dummy_image.view().handle(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        }
    );

    std::array desc_writes = generate_array(layout_bindings, desc_write_from_binding);
    auto& [sampler_desc_write, texarr_desc_write] = desc_writes;

    const VkDescriptorImageInfo sampler_info{ .sampler = texarr.sampler.handle() };
    sampler_desc_write.pImageInfo = &sampler_info;
    texarr_desc_write.pImageInfo = texarr.texture_array_infos.data();

    texarr.texarr_descriptors.resize(frame_count);
    texarr.texarr_descriptor_pool.create_sets(texarr.texarr_descriptors, texarr.texarr_descriptor_layout.handle());

    for (auto descriptor : texarr.texarr_descriptors) {
        for (auto& desc_write : desc_writes) {
            desc_write.dstSet = descriptor;
        }
        vkUpdateDescriptorSets(device.handle(), static_cast<u32_t>(desc_writes.size()), desc_writes.data(), 0, nullptr);
    }

    return texarr;
}

}