#include "renderer.hpp"

#include "vkw/queue/queue_fun.hpp"

namespace dry {

template<u32_t Set>
auto extract_set_bindings(std::vector<asset::vk_shader_data::layout_binding_info>& in) {
    return extract_shader_layouts < [](asset::vk_shader_data::layout_binding_info& binding) { return binding.set == Set; } > (in);
}

bool assure_descriptor_bindings(std::span<const asset::vk_shader_data::layout_binding_info> target, std::span<const asset::vk_shader_data::layout_binding_info> filter) {
    for (const auto& binding : filter) {
        auto binding_eq_lambda = [&binding](const asset::vk_shader_data::layout_binding_info& el) {
            return binding.binding == el.binding && binding.count == el.count &&
                binding.set == el.set && binding.stage == el.stage && binding.type == el.type;
        };

        if (std::find_if(target.begin(), target.end(), binding_eq_lambda) == target.end()) {
            return false;
        }
    }
    return true;
}

vulkan_renderer::resource_id vulkan_renderer::create_texture(const asset::texture_source& tex) {
    const u32_t mip_levels = static_cast<u32_t>(std::log2((std::max)(tex.width, tex.height)));

    auto texture = create_sampled_texture(_device, _graphics_queue, _transfer_queue, tex.pixel_data,
        { tex.width, tex.height }, mip_levels, asset::texture_vk_format(tex)
    );

    // _resources.texture_refcount.emplace(0);
    const auto tex_ind = _resources.textures.emplace(std::move(texture));

    _texarr.add_texture(static_cast<u32_t>(tex_ind), _resources.textures[tex_ind].view().handle());

    return static_cast<resource_id>(tex_ind);
}

vulkan_renderer::resource_id vulkan_renderer::create_mesh(const asset::mesh_asset& mesh) {
    renderer_resources::vertex_buffer mesh_buffer;
    // TODO : buffer for each mesh currently, do: allocate into one and offset into it
    mesh_buffer.indices = vkw::create_local_buffer(_transfer_queue, _device,
        std::span{ mesh.indices }, VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );
    mesh_buffer.vertices = vkw::create_local_buffer(_transfer_queue, _device, 
        std::span{ mesh.vertices }, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );

    return static_cast<resource_id>(_resources.vertex_buffers.emplace(std::move(mesh_buffer)));
}

vulkan_renderer::resource_id vulkan_renderer::create_shader(const asset::shader_asset& shader) {
    renderer_resources::shader_pipeline new_pipeline;
    new_pipeline.shared_descriptors.resize(_image_count);

    std::vector<VkDescriptorSetLayout> desc_layouts;
    asset::vk_shader_data shader_data = asset::shader_vk_info(shader);

    // global bindings, must be present
    {
        auto set0_bindings = extract_set_bindings<0>(shader_data.layout_bindings);
        if (!assure_descriptor_bindings(set0_bindings, instanced_pass::layout_bindings)) {
            LOG_ERR("Required layout bindings at set 0 are not present, invalid shader");
            dbg::panic();
        }
        // global present, ok
        desc_layouts.push_back(_instanced_pass.instanced_descriptor_layout.handle());
    }

    // shared resources
    {
        auto add_shared_desc_lambda =
            [&target_desc = new_pipeline.shared_descriptors, &desc_layouts](std::vector<VkDescriptorSet>& src_desc, VkDescriptorSetLayout layout) {

            desc_layouts.push_back(layout);
            for (auto i = 0u; i < target_desc.size(); ++i) {
                target_desc[i].push_back(src_desc[i]);
            }
        };

        auto set1_bindings = extract_set_bindings<1>(shader_data.layout_bindings);
        if (assure_descriptor_bindings(set1_bindings, texture_array::layout_bindings)) {
            add_shared_desc_lambda(_texarr.texarr_descriptors, _texarr.texarr_descriptor_layout.handle());
        }
    }

    // rest
    {
        auto set2_bindings = extract_set_bindings<2>(shader_data.layout_bindings);
        new_pipeline.pipeline_data = pipeline_resources{ _device, _image_count, set2_bindings };

        if (new_pipeline.pipeline_data.has_resources()) {
            desc_layouts.push_back(new_pipeline.pipeline_data.descriptor_layout());
        }
    }

    const auto vert_input_info = vertex_bindings_to_input(shader_data.vertex_descriptors);

    std::vector<vkw::vk_shader_module> shader_modules;
    shader_modules.reserve(shader.oth_stages.size() + 1); // NOTE : assuming vertex present

    shader_modules.emplace_back(_device, shader.vert_stage.spirv, asset::shader_vk_stage(shader.vert_stage.stage));
    for (const auto& shader_stage : shader.oth_stages) {
        shader_modules.emplace_back(_device, shader_stage.spirv, asset::shader_vk_stage(shader_stage.stage));
    }

    new_pipeline.pipeline = vkw::vk_pipeline_graphics{ _device, _render_pass, _extent, shader_modules,
        std::span{ &vert_input_info.binding_desc, 1 }, vert_input_info.attribute_desc, desc_layouts
    };

    if (new_pipeline.pipeline_data.has_materials()) {
        new_pipeline.material_update_status.resize(_image_count, true);
    }

    // _resources.pipeline_refcount.emplace(0);
    return static_cast<resource_id>(_resources.pipelines.emplace(std::move(new_pipeline)));
}

vulkan_renderer::renderable_id vulkan_renderer::create_renderable(resource_id material, resource_id mesh) {
    // _resources.material_refcount[material] += 1;
    // _resources.vertex_refcount[mesh] += 1;

    // NOTE : keep in mind narrowing conversion happening
    const auto& material_data = *_resources.materials[material];

    renderable_id rend;
    rend.mesh = static_cast<u16_t>(mesh);
    rend.pipeline = static_cast<u16_t>(material_data.pipeline_index);
    rend.renderable = static_cast<u32_t>(_resources.pipelines[rend.pipeline].renderables[mesh].emplace(_default_transform, static_cast<u32_t>(material_data.local_index)));

    return rend;
}

void vulkan_renderer::destroy_renderable(renderable_id rend) {
    _resources.pipelines[rend.pipeline].renderables[rend.mesh].remove(rend.renderable);

    // TODO : cleanup and refcounting
}

void vulkan_renderer::update_renderable_transform(renderable_id rend, const object_transform& trans) {
    _resources.pipelines[rend.pipeline].renderables[rend.mesh][rend.renderable].transform = trans;
}
void vulkan_renderer::update_camera_transform(const camera_transform& trans) {
    _resources.cam_transform = trans;
}

}