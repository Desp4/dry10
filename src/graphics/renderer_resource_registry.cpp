#include "renderer_resource_registry.hpp"

#include <cmath>

#include "vk_initers.hpp"

namespace dry {

renderer_resource_registry::renderer_resource_registry(
    const vkw::vk_device& device, const vkw::vk_render_pass& render_pass,
    const vkw::vk_queue_graphics& graphics_queue, const vkw::vk_queue_transfer& transfer_queue,
    u32_t image_count) :
    _device{ &device },
    _render_pass{ &render_pass },
    _graphics_queue{ &graphics_queue },
    _transfer_queue{ &transfer_queue },
    _image_count{ image_count },
    _per_frame_buffers(_image_count)
{
}

void renderer_resource_registry::set_descriptor_layout_stages(std::span<const stage_descriptor_layout> stage_layouts) {
    _stage_layouts.assign(stage_layouts.begin(), stage_layouts.end());
}

void renderer_resource_registry::set_surface_extent(VkExtent2D extent) {
    _surface_extent = extent;
}

renderer_resource_registry::index_type renderer_resource_registry::allocate_pipeline(const asset::shader_source& shader, layout_stage_mask_t stage_mask) {
    std::vector<vkw::vk_shader_module> shader_modules;
    shader_modules.reserve(shader.oth_stages.size() + 1); // NOTE : assuming vertex present
    shader_modules.emplace_back(*_device, shader.vert_stage.spirv, asset::shader_vk_stage(shader.vert_stage.stage));
    for (const auto& shader_stage : shader.oth_stages) {
        shader_modules.emplace_back(*_device, shader_stage.spirv, asset::shader_vk_stage(shader_stage.stage));
    }

    std::vector<VkDescriptorSetLayoutBinding> exclude_bindings;
    std::vector<VkDescriptorSetLayout> desc_layouts;
    for (const auto& stage_layout : _stage_layouts) {
        if (stage_layout.mask & stage_mask) {
            desc_layouts.push_back(stage_layout.layout);
            for (const auto& binding : stage_layout.exclude_bindings) {
                exclude_bindings.push_back(binding);
            }
        }
    }

    pipeline_data new_pipeline_data;
    const asset::vk_shader_data shader_data = asset::shader_vk_info(shader, exclude_bindings);
    // not empty after exclude warrants a descriptor pool and layout
    if (shader_data.layout_bindings.size() != 0) {
        vkw::vk_descriptor_layout pipeline_desc_layout{ *_device, shader_data.layout_bindings };
        desc_layouts.push_back(pipeline_desc_layout.handle());

        std::vector<VkDescriptorPoolSize> pool_sizes;
        if (shader_data.buffer_infos.size() != 0) {
            pool_sizes.emplace_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _target_descriptor_pool_capacity);
        }
        if (shader_data.comb_sampler_infos.size() != 0) {
            pool_sizes.emplace_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _target_descriptor_pool_capacity);
        }

        pipeline_descriptor_sets pipeline_desc_sets;
        pipeline_desc_sets.pool = vkw::vk_descriptor_superpool{ *_device, pool_sizes, _target_descriptor_pool_capacity, pipeline_desc_layout.handle() };
        pipeline_desc_sets.frame_descriptors.resize(_image_count);

        new_pipeline_data.layout = static_cast<index_type>(_layouts.emplace(std::move(pipeline_desc_layout)));
        new_pipeline_data.descriptor_sets = static_cast<index_type>(_pipeline_descriptors.emplace(std::move(pipeline_desc_sets)));
    }
    else {
        new_pipeline_data.layout = null_index;
        new_pipeline_data.descriptor_sets = null_index;
    }

    new_pipeline_data.pipeline = vkw::vk_pipeline_graphics{
        *_device, *_render_pass, _surface_extent,
        shader_modules, shader_data, desc_layouts
    };

    _pipeline_reflect_datas.emplace(std::move(shader_data)); // again, same index on insertion, can discard the return
    return static_cast<index_type>(_pipelines.emplace(std::move(new_pipeline_data)));
}

renderer_resource_registry::index_type renderer_resource_registry::allocate_vertex_buffer(const asset::mesh_source& mesh) {
    vertex_data buffer;
    buffer.indices = _transfer_queue->create_local_buffer(mesh.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    buffer.vertices = _transfer_queue->create_local_buffer(mesh.vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    return static_cast<index_type>(_vertex_buffers.emplace(std::move(buffer)));
}

renderer_resource_registry::index_type renderer_resource_registry::allocate_texture(const asset::texture_source& texture) {
    texture_data sampler;
    const u32_t mip_levels = static_cast<u32_t>(std::log2((std::max)(texture.width, texture.height)));

    vkw::vk_buffer staging_buffer{ *_device,
        texture.pixel_data.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };
    staging_buffer.write(texture.pixel_data);

    sampler.texture = vkw::vk_image_view_pair{ *_device,
        VkExtent2D{ texture.width, texture.height },
        mip_levels,
        VK_SAMPLE_COUNT_1_BIT,
        asset::texture_vk_format(texture),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    };

    _graphics_queue->transition_image_layout(sampler.texture.image(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    _transfer_queue->copy_buffer_to_image(staging_buffer.handle(), sampler.texture.image());
    _graphics_queue->generate_mip_maps(sampler.texture.image());

    sampler.sampler = vkw::vk_tex_sampler{ *_device, mip_levels };

    return static_cast<index_type>(_textures.emplace(std::move(sampler)));
}

renderer_resource_registry::material_index renderer_resource_registry::allocate_material(index_type pipeline, std::span<const index_type> textures) {
    material_data new_material{ .textures{textures.begin(), textures.end()}, .descriptor = VK_NULL_HANDLE };
    // create material descriptor, checking combined samplers only for now TODO :
    auto& parent_pipeline = _pipelines[pipeline];
    const auto& shader_data = _pipeline_reflect_datas[pipeline];

    assert(textures.size() == shader_data.comb_sampler_infos.size());
    if (shader_data.comb_sampler_infos.size() != 0) {
        auto& desc_sets = _pipeline_descriptors[parent_pipeline.descriptor_sets];

        std::vector<VkWriteDescriptorSet> desc_writes;
        desc_writes.reserve(textures.size());

        new_material.descriptor = desc_sets.pool.get_descriptor_set();
        std::vector<VkDescriptorImageInfo> image_infos;
        image_infos.reserve(textures.size());

        for (auto i = 0u; i < textures.size(); ++i) {
            const auto& sampler_info = shader_data.comb_sampler_infos[i];
            auto desc_write = desc_write_from_binding(shader_data.layout_bindings[sampler_info.binding_ind]);
            desc_write.dstSet = new_material.descriptor;

            const auto& sampler = _textures[textures[i]];
            VkDescriptorImageInfo img_info{};
            img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            img_info.imageView = sampler.texture.view().handle();
            img_info.sampler = sampler.sampler.handle();

            image_infos.push_back(std::move(img_info));
            desc_write.pImageInfo = &image_infos.back();

            desc_writes.push_back(std::move(desc_write));
        }
        vkUpdateDescriptorSets(_device->handle(), static_cast<u32_t>(desc_writes.size()), desc_writes.data(), 0, nullptr);
    }

    return { pipeline, static_cast<index_type>(parent_pipeline.materials.emplace(std::move(new_material))) };
}

renderer_resource_registry::renderable_index renderer_resource_registry::allocate_renderable(material_index material, index_type mesh) {
    auto& parent_material = _pipelines[material.pipeline].materials[material.material];
    const auto& shader_data = _pipeline_reflect_datas[material.pipeline];

    renderable_data renderable{ .descriptor = null_index };
    renderable.buffers.reserve(shader_data.buffer_infos.size());
    for (const auto& buffer_info : shader_data.buffer_infos) {
        renderable.buffers.push_back(
            allocate_renderable_buffer(shader_data.layout_bindings[buffer_info.binding_ind].descriptorType, buffer_info.info.range, buffer_info.info.offset)
        );
    }

    if (renderable.buffers.size() != 0) {
        renderable.descriptor = allocate_renderable_descriptor(material.pipeline, renderable.buffers);
    }    

    auto mesh_it = std::lower_bound(parent_material.mesh_groups.begin(), parent_material.mesh_groups.end(), mesh,
        [](auto& elem, auto val) { return elem.mesh == val; }
    );

    if (mesh_it == parent_material.mesh_groups.end()) {
        parent_material.mesh_groups.push_back(mesh_group{ .mesh = mesh });
        mesh_it = parent_material.mesh_groups.begin() + (parent_material.mesh_groups.size() - 1);
    }
    else if (mesh_it->mesh != mesh) {
        mesh_it = parent_material.mesh_groups.insert(mesh_it, mesh_group{ .mesh = mesh });
    }

    renderable_index ret_rend{ .material = material,.mesh = mesh };
    ret_rend.renderable = static_cast<index_type>(mesh_it->renderables.emplace(std::move(renderable)));
    return ret_rend;
}

renderer_resource_registry::index_type renderer_resource_registry::allocate_renderable_buffer(VkDescriptorType descriptor_type, VkDeviceSize range, VkDeviceSize offset) {
    index_type ret_index{};
    for (auto& frame_buffers : _per_frame_buffers) {
        ret_index = static_cast<index_type>(frame_buffers.emplace(*_device,
            range,
            descriptor_type_to_buffer_usage(descriptor_type),
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT // NOTE : writable -> local and coherent
        ));
    }
    return ret_index;
}

renderer_resource_registry::index_type renderer_resource_registry::allocate_renderable_descriptor(index_type pipeline, std::span<const index_type> buffers) {
    const auto& shader_data = _pipeline_reflect_datas[pipeline];
    auto& pipeline_descs = _pipeline_descriptors[_pipelines[pipeline].descriptor_sets];
    assert(shader_data.buffer_infos.size() == buffers.size());

    std::vector<VkWriteDescriptorSet> desc_writes(buffers.size());
    for (auto i = 0u; i < desc_writes.size(); ++i) {
        desc_writes[i] = desc_write_from_binding(shader_data.layout_bindings[shader_data.buffer_infos[i].binding_ind]);
    }

    std::vector<VkDescriptorBufferInfo> buffer_infos;
    buffer_infos.reserve(shader_data.buffer_infos.size());
    for (auto buffer_info : shader_data.buffer_infos) {
        buffer_infos.push_back(buffer_info.info);
    }

    index_type ret_desc{ null_index };
    for (auto i = 0u; i < _image_count; ++i) {
        VkDescriptorSet desc_set = pipeline_descs.pool.get_descriptor_set();
        for (auto j = 0u; j < buffers.size(); ++i) {
            buffer_infos[j].buffer = _per_frame_buffers[i][buffers[j]].handle();
            desc_writes[j].pBufferInfo = &buffer_infos[j];
            desc_writes[j].dstSet = desc_set;
        }
        vkUpdateDescriptorSets(_device->handle(), static_cast<u32_t>(desc_writes.size()), desc_writes.data(), 0, nullptr);
        ret_desc = static_cast<index_type>(pipeline_descs.frame_descriptors[i].emplace(desc_set)); // all indices for each frame are the same
    }
    return ret_desc;
}

}