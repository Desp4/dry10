#include "renderer.hpp"

namespace dry {

vulkan_renderer::resource_id vulkan_renderer::create_texture(const asset::texture_source& tex) {
    // const u32_t mip_levels = static_cast<u32_t>(std::log2((std::max)(tex.width, tex.height)));
    constexpr u32_t mip_levels = unshaded_pass::mip_levels; // NOTE

    vkw::vk_buffer staging_buffer{ _device,
        tex.pixel_data.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };
    staging_buffer.write(tex.pixel_data);

    vkw::vk_image_view_pair texture{ _device,
        VkExtent2D{ tex.width, tex.height },
        mip_levels,
        VK_SAMPLE_COUNT_1_BIT,
        asset::texture_vk_format(tex),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    };

    _graphics_queue.transition_image_layout(texture.image(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    _transfer_queue.copy_buffer_to_image(staging_buffer.handle(), texture.image());
    _graphics_queue.generate_mip_maps(texture.image());

    _resources.texture_refcount.emplace(0);
    const auto tex_ind = _resources.textures.emplace(std::move(texture));

    update_unshaded_textures(_unshaded_pass, static_cast<u32_t>(tex_ind), _resources.textures[tex_ind].view().handle());

    return static_cast<resource_id>(tex_ind);
}

vulkan_renderer::resource_id vulkan_renderer::create_mesh(const asset::mesh_asset& mesh) {
    renderer_resources::vertex_buffer mesh_buffer;
    // TODO : buffer for each mesh currently, do: allocate into one and offset into it
    mesh_buffer.indices = _transfer_queue.create_local_buffer(mesh.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    mesh_buffer.vertices = _transfer_queue.create_local_buffer(mesh.vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    _resources.vertex_refcount.emplace(0);
    return static_cast<resource_id>(_resources.vertex_buffers.emplace(std::move(mesh_buffer)));
}

vulkan_renderer::resource_id vulkan_renderer::create_shader(const asset::shader_asset& shader) {
    std::vector<vkw::vk_shader_module> shader_modules;
    shader_modules.reserve(shader.oth_stages.size() + 1); // NOTE : assuming vertex present

    shader_modules.emplace_back(_device, shader.vert_stage.spirv, asset::shader_vk_stage(shader.vert_stage.stage));
    for (const auto& shader_stage : shader.oth_stages) {
        shader_modules.emplace_back(_device, shader_stage.spirv, asset::shader_vk_stage(shader_stage.stage));
    }

    std::vector<VkDescriptorSetLayout> desc_layouts;
    asset::vk_shader_data shader_data = asset::shader_vk_info(shader);

    // chip away all stage specific attributes, unshaded only currently
    struct pass_layout_data {
        std::span<const asset::vk_shader_data::layout_binding_info> exclude_bindings;
        VkDescriptorSetLayout layout;
    };
    const std::array fixed_pass_layouts{ pass_layout_data{
        .exclude_bindings{ unshaded_pass::layout_bindings },
        .layout{ _unshaded_pass.unshaded_descriptor_layout.handle() }
    }};

    for (const auto& layout : fixed_pass_layouts) {
        desc_layouts.push_back(layout.layout);

        for (const auto& binding : layout.exclude_bindings) {
            // eh, compare everything except stride
            auto relaxed_binding_eq_lambda = [&val = binding](const auto& el) {
                return el.binding == val.binding && el.set == val.set && el.count == val.count && el.stage == val.stage && el.type == val.type;
            };

            const auto it = std::find_if(shader_data.layout_bindings.begin(), shader_data.layout_bindings.end(), relaxed_binding_eq_lambda);
            if (it == shader_data.layout_bindings.end()) {
                LOG_ERR("Required layout binding not found, shader incopatible");
                dbg::panic();
            }

            shader_data.layout_bindings.erase(it);
        }
    }

    renderer_resources::shader_pipeline pipeline;
    // if after excluding passes layout bindings not empty - need a desc layout
    if (shader_data.layout_bindings.size() != 0) {
        // create descriptor layout
        std::vector<VkDescriptorSetLayoutBinding> shader_bindings;
        shader_bindings.reserve(shader_data.layout_bindings.size());

        for (const auto& binding : shader_data.layout_bindings) {
            VkDescriptorSetLayoutBinding layout{};
            layout.binding = binding.binding;
            layout.descriptorCount = binding.count;
            layout.descriptorType = binding.type;
            layout.stageFlags = binding.stage;
            shader_bindings.push_back(layout);
        }

        pipeline.layout = vkw::vk_descriptor_layout{ _device, shader_bindings };
        desc_layouts.push_back(pipeline.layout.handle());

        std::vector<VkDescriptorPoolSize> pool_sizes;
        auto try_add_pool_size_lambda = [&shader_data, &pool_sizes](VkDescriptorType desc_type) {
            auto find_desc_lambda = [desc_type](const auto& el) {return el.type == desc_type; };
            if (std::find_if(shader_data.layout_bindings.begin(), shader_data.layout_bindings.end(), find_desc_lambda) != shader_data.layout_bindings.end()) {
                pool_sizes.emplace_back(desc_type, _primary_descriptor_pool_capacity);
            }
        };

        // NOTE TODO : not bothering to create those descriptors and resources, basically this branch is for nothing without that
        // try_add_pool_size_lambda(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        // ... and the rest
        pipeline.descriptor_pool = vkw::vk_descriptor_superpool{ _device, pool_sizes,_primary_descriptor_pool_capacity, pipeline.layout.handle() };
    }

    const auto& vert_input_data = select_vertex_input(shader_data.vertex_descriptors, unshaded_pass::vertex_inputs);
    pipeline.pipeline = vkw::vk_pipeline_graphics{ _device, _render_pass, _extent, shader_modules,
        vert_input_data.binding_desc, vert_input_data.attribute_desc, desc_layouts
    };

    _resources.pipeline_refcount.emplace(0);
    return static_cast<resource_id>(_resources.pipelines.emplace(std::move(pipeline)));
}

vulkan_renderer::resource_id vulkan_renderer::create_material(resource_id pipeline, std::span<const resource_id> textures) {
    _resources.pipeline_refcount[pipeline] += 1;
    for (auto tex : textures) {
        _resources.texture_refcount[tex] += 1;
    }

    _resources.material_refcount.emplace(0);
    return static_cast<resource_id>(_resources.materials.emplace(pipeline, std::vector<resource_id>{ textures.begin(), textures.end() }));
}

vulkan_renderer::renderable_id vulkan_renderer::create_renderable(resource_id material, resource_id mesh) {
    _resources.material_refcount[material] += 1;
    _resources.vertex_refcount[mesh] += 1;

    // NOTE : keep in mind narrowing conversion happening
    renderable_id rend;
    rend.mesh = static_cast<u16_t>(mesh);
    rend.pipeline = static_cast<u16_t>(_resources.materials[material].pipeline);
    rend.renderable = static_cast<u32_t>(_resources.pipelines[rend.pipeline].renderables[mesh].emplace(&_default_transform, material));

    return rend;
}

void vulkan_renderer::destroy_renderable(renderable_id rend) {
    _resources.pipelines[rend.pipeline].renderables[rend.mesh].remove(rend.renderable);

    // TODO : cleanup and refcounting
}

void vulkan_renderer::bind_renderable_transform(renderable_id rend, const object_transform& transform) {
    _resources.pipelines[rend.pipeline].renderables[rend.mesh][rend.renderable].transform_ptr = &transform;
}

}