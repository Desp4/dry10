#include "unshaded_pass.hpp"

#include "vk_initers.hpp"

namespace dry {

template<typename T, std::size_t N, typename Functor>
constexpr auto generate_array(const std::array<T, N>& range, Functor fun) {
    using U = decltype(fun(std::declval<T>()));
    std::array<U, N> ret;
    for (auto i = 0u; i < N; ++i) {
        ret[i] = fun(range[i]);
    }
    return ret;
}

unshaded_pass create_unshaded_pass(const vkw::vk_device& device, const vkw::vk_queue_graphics& graphics_queue, u32_t frame_count) {
    unshaded_pass pass;

    // TODO : pulled it out of ass
    pass.dummy_image = vkw::vk_image_view_pair{
        device, { 512, 512 }, unshaded_pass::mip_levels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    };
    graphics_queue.transition_image_layout(pass.dummy_image.image(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    graphics_queue.generate_mip_maps(pass.dummy_image.image());

    constexpr std::array layout_bindings = generate_array(unshaded_pass::layout_bindings, layout_binding_from_reflect_info);

    auto desc_pool_lambda = [frame_count](decltype(layout_bindings)::value_type val) -> VkDescriptorPoolSize {
        return VkDescriptorPoolSize{ .type = val.descriptorType, .descriptorCount = frame_count };
    };

    pass.sampler = vkw::vk_tex_sampler{ device, unshaded_pass::mip_levels };
    pass.unshaded_descriptor_layout = vkw::vk_descriptor_layout{ device, layout_bindings };

    pass.instance_buffers.reserve(frame_count);
    for (auto i = 0u; i < frame_count; ++i) {
        pass.instance_buffers.emplace_back(
            device, sizeof(unshaded_pass::instance_input) * unshaded_pass::combined_instance_buffer_count,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT // NOTE : local memory
        );
    }

    const std::array desc_pool_sizes = generate_array(layout_bindings, desc_pool_lambda);
    pass.unshaded_descriptor_pool = vkw::vk_descriptor_pool{ device, desc_pool_sizes, frame_count };

    pass.unshaded_descriptors.resize(frame_count);
    pass.texture_update_status.resize(frame_count, true);
    pass.unshaded_descriptor_pool.create_sets(pass.unshaded_descriptors, pass.unshaded_descriptor_layout.handle());

    std::array desc_writes = generate_array(layout_bindings, desc_write_from_binding);
    auto& camera_desc_write = desc_writes[0];
    auto& sampler_desc_write = desc_writes[1];
    auto& texarr_desc_write = desc_writes[2];

    pass.texture_array_infos.resize(unshaded_pass::texture_array_size,
        VkDescriptorImageInfo{
            .sampler = VK_NULL_HANDLE,
            .imageView = pass.dummy_image.view().handle(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        }
    );
    texarr_desc_write.pImageInfo = pass.texture_array_infos.data();

    const VkDescriptorImageInfo sampler_info{ .sampler = pass.sampler.handle() };
    sampler_desc_write.pImageInfo = &sampler_info;

    pass.camera_transforms.reserve(frame_count);
    for (auto i = 0u; i < frame_count; ++i) {
        const auto& frame_ubo = pass.camera_transforms.emplace_back(
            device, sizeof(camera_transform),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        VkDescriptorBufferInfo camera_buffer_info{};
        camera_buffer_info.buffer = frame_ubo.handle();
        camera_buffer_info.range = frame_ubo.size();
        camera_buffer_info.offset = 0;

        camera_desc_write.pBufferInfo = &camera_buffer_info;

        for (auto& desc_write : desc_writes) {
            desc_write.dstSet = pass.unshaded_descriptors[i];
        }

        vkUpdateDescriptorSets(device.handle(), static_cast<u32_t>(desc_writes.size()), desc_writes.data(), 0, nullptr);
    }

    return pass;
}

void update_unshaded_textures(unshaded_pass& pass, u32_t index, VkImageView view) {
    pass.texture_array_infos[index].imageView = view;

    for (auto i = 0u; i < pass.texture_update_status.size(); ++i) {
        pass.texture_update_status[i] = false;
    }
}

void update_texture_descriptors(const vkw::vk_device& device, unshaded_pass& pass, u32_t frame) {
    auto desc_write = desc_write_from_binding(layout_binding_from_reflect_info(unshaded_pass::texarr_layout_binding));
    desc_write.pImageInfo = pass.texture_array_infos.data();
    desc_write.dstSet = pass.unshaded_descriptors[frame];

    vkUpdateDescriptorSets(device.handle(), 1, &desc_write, 0, nullptr);

    pass.texture_update_status[frame] = true;
}

}