#include "instanced_pass.hpp"

#include "util/util.hpp"
#include "vk_initers.hpp"

namespace dry {

instanced_pass create_instanced_pass(const vkw::vk_device& device, u32_t frame_count) {
    instanced_pass pass;

    constexpr std::array layout_bindings = generate_array(instanced_pass::layout_bindings, layout_binding_from_reflect_info);

    auto desc_pool_lambda = [frame_count](decltype(layout_bindings)::value_type val) -> VkDescriptorPoolSize {
        return VkDescriptorPoolSize{ .type = val.descriptorType, .descriptorCount = frame_count };
    };

    pass.instanced_descriptor_layout = vkw::vk_descriptor_layout{ device, layout_bindings };

    pass.instance_staging_buffer = vkw::vk_buffer{
        device, sizeof(instanced_pass::instance_input) * instanced_pass::combined_instance_buffer_count,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY
    };

    const std::array desc_pool_sizes = generate_array(layout_bindings, desc_pool_lambda);
    pass.instanced_descriptor_pool = vkw::vk_descriptor_pool{ device, desc_pool_sizes, frame_count };

    pass.instance_descriptors.resize(frame_count);
    pass.instanced_descriptor_pool.create_sets(pass.instance_descriptors, pass.instanced_descriptor_layout.handle());

    std::array desc_writes = generate_array(layout_bindings, desc_write_from_binding);
    auto& [camera_desc_write, instance_desc_write] = desc_writes;

    pass.camera_transforms.reserve(frame_count);
    pass.instance_buffers.reserve(frame_count);
    for (auto i = 0u; i < frame_count; ++i) {
        const auto& camera_ubo = pass.camera_transforms.emplace_back(
            device, sizeof(camera_transform),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        VkDescriptorBufferInfo camera_buffer_info{};
        camera_buffer_info.buffer = camera_ubo.handle();
        camera_buffer_info.range = camera_ubo.size();
        camera_buffer_info.offset = 0;

        camera_desc_write.pBufferInfo = &camera_buffer_info;

        const auto& instance_buffer = pass.instance_buffers.emplace_back(
            device, sizeof(instanced_pass::instance_input) * instanced_pass::combined_instance_buffer_count,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY
        );

        VkDescriptorBufferInfo instance_buffer_info{};
        instance_buffer_info.buffer = instance_buffer.handle();
        instance_buffer_info.range = instance_buffer.size();
        instance_buffer_info.offset = 0;

        instance_desc_write.pBufferInfo = &instance_buffer_info;

        for (auto& desc_write : desc_writes) {
            desc_write.dstSet = pass.instance_descriptors[i];
        }

        vkUpdateDescriptorSets(device.handle(), static_cast<u32_t>(desc_writes.size()), desc_writes.data(), 0, nullptr);
    }

    return pass;
}

}