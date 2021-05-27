#ifdef WIN32
  #define VK_USE_PLATFORM_WIN32_KHR
#endif
#include "renderer.hpp"

#include <algorithm>
// tmp for tests
#include <glm/gtc/matrix_transform.hpp>

#include "dbg/log.hpp"

#include "vkw/device/vk_functions.hpp"

namespace dry {

// TODO : maybe move these two
static constexpr VkWriteDescriptorSet desc_write_from_binding(VkDescriptorSetLayoutBinding binding) {
    VkWriteDescriptorSet desc_write{};

    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstBinding = binding.binding;
    desc_write.descriptorType = binding.descriptorType;
    desc_write.descriptorCount = binding.descriptorCount;
    return desc_write;
}

static constexpr VkBufferUsageFlags descriptor_type_to_buffer_usage(VkDescriptorType type) {
    switch (type) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    default: LOG_ERR("Can't initialize a buffer from this descriptor"); dbg::panic(); // TODO : fmt type into log err
    }
}

vulkan_renderer::vulkan_renderer(const wsi::window& window) {
    const auto& instance = vk_instance();
    _surface = vkw::vk_surface{ instance, window };

    const auto phys_device = find_physical_device();
    const auto queue_infos = populate_queue_infos(phys_device);
    _device = vkw::vk_device{ phys_device, queue_infos.device_queue_infos, _device_extensions, _device_features };

    _present_queue = vkw::vk_queue{ _device, queue_infos.queue_init_infos[0].family_ind, queue_infos.queue_init_infos[0].queue_ind };
    _graphics_queue = vkw::vk_queue_graphics{ _device, queue_infos.queue_init_infos[1].family_ind, queue_infos.queue_init_infos[1].queue_ind };
    _transfer_queue = vkw::vk_queue_transfer{ _device, queue_infos.queue_init_infos[2].family_ind, queue_infos.queue_init_infos[2].queue_ind };

    const auto surface_capabilities = _device.surface_capabilities(_surface.handle());
    _image_count = surface_capabilities.maxImageCount == 0 ?
        surface_capabilities.minImageCount + 1 :
        (std::min)(surface_capabilities.minImageCount + 1, surface_capabilities.maxImageCount);

    _swapchain = vkw::vk_swapchain_present{
        _device, _surface.handle(),
        surface_capabilities.currentExtent, surface_capabilities.currentTransform, _image_count,
        _primary_image_format, _primary_image_colorspace, _primary_image_present_mode
    };
    _render_pass = vkw::vk_render_pass{
        _device, surface_capabilities.currentExtent,
        vkw::render_pass_flags::color | vkw::render_pass_flags::depth | vkw::render_pass_flags::msaa,
        _primary_msaa_sample_count, _primary_image_format, _primary_depth_format
    };
    _render_pass.create_framebuffers(_swapchain.swap_views());

    create_global_descriptors();
    _buffers.resize(_image_count);

    _cmd_buffers.resize(_image_count);
    for (auto& cmd_buffer : _cmd_buffers) {
        cmd_buffer = vkw::vk_cmd_buffer{ _device, _present_queue.cmd_pool() };
    }
}

vulkan_renderer::renderable_hash vulkan_renderer::create_renderable(const asset::mesh_asset& mesh, const material_asset& material) {
    if (!_pipelines.contains(material.shader->hash)) {
        _pipelines[material.shader->hash] = create_pipeline(*material.shader);
    }
    
    auto& pipeline = _pipelines[material.shader->hash];
    const material_hash mat_hash{ material };
    if (!pipeline.materials.contains(mat_hash)) {
        material_data mat_data;
        // TODO : textures are bound in order of appearance, validity should also be enforced by material
        mat_data.texs.reserve(material.textures.size());
        for (const auto& tex : material.textures) {
            if (!_samplers.contains(tex->hash)) {
                _samplers[tex->hash] = create_sampler(*tex);
            }
            mat_data.texs.push_back(tex->hash);
        }
        mat_data.descriptor = create_material_descriptor(pipeline, mat_data); // TODO : errors if no descriptor to create

        pipeline.materials[mat_hash] = std::move(mat_data);
    }
    auto& material_data = pipeline.materials[mat_hash];

    if (!material_data.renderables.contains(mesh.hash)) {
        if (!_meshes.contains(mesh.hash)) {
            _meshes[mesh.hash] = create_vertex_buffers(mesh);
        }
        material_data.renderables[mesh.hash] = sparse_set<renderable>{};
    }

    std::vector<buffer_hash> buffer_hs;
    buffer_hs.reserve(pipeline.shader_data.buffer_infos.size());
    // assuming all the remaining buffers are per object and per frame
    for (const auto& buffer_info : pipeline.shader_data.buffer_infos) {
        buffer_hs.push_back(create_buffer(pipeline.shader_data, buffer_info));
    }

    renderable new_renderable{};
    if (buffer_hs.size() != 0) {
        new_renderable.desc_h = create_renderable_frame_descriptors(pipeline, buffer_hs);
    }
    else {
        new_renderable.desc_h = persistent_index_null;
    }
    

    const auto rend_index = material_data.renderables[mesh.hash].emplace(std::move(new_renderable));

    renderable_hash new_renderable_hash;
    new_renderable_hash.shader = material.shader->hash;
    new_renderable_hash.material = std::move(mat_hash);
    new_renderable_hash.mesh = mesh.hash;
    new_renderable_hash.index = rend_index;
    
    _renderable_buffer_map[new_renderable_hash] = std::move(buffer_hs);
    return new_renderable_hash;
}

void vulkan_renderer::submit_frame() {
    VkDeviceSize offsets[1]{ 0 };

    const auto frame_index = _swapchain.acquire_frame();
    const auto& cmd_buffer = _cmd_buffers[frame_index];
    const auto cmd_buffer_h = cmd_buffer.handle();

    // write buffers here, tmp for testing
    {
        glm::vec3 cam_pos{ 0.0f, -6.0f, -10.0f };
        glm::mat4 view = glm::translate(glm::mat4(1.0f), cam_pos);
        glm::mat4 proj = glm::perspective(glm::radians(70.0f), 1.0f, 0.1f, 200.0f);
        proj[1][1] *= -1;

        camera_transform cam_trans{};
        cam_trans.proj = proj;
        cam_trans.view = view;
        cam_trans.viewproj = proj * view;
        _camera_ubos[frame_index].write(cam_trans);

        glm::mat4 translation = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, 0.0f));
        glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2f, 0.2f, 0.2f));
        model_transform model_trans = translation * scale;

        // only writing first one, one object
        _model_mat_storage[frame_index].write(model_trans);
    }

    vkResetCommandBuffer(cmd_buffer_h, 0);
    _render_pass.start_cmd_pass(cmd_buffer, frame_index);

    // recording
    u32_t instance_count = 0;
    for (const auto& pipeline : _pipelines) {
        pipeline.second.pipeline.bind_pipeline(cmd_buffer_h);
        const auto pipeline_layout = pipeline.second.pipeline.layout();

        // bind global descs TODO : rebinding pipeline should keep descs bound if compatible, possibly doing redundant work
        vkCmdBindDescriptorSets(
            cmd_buffer_h, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
            0, 1, &_global_descriptors[frame_index], 0, nullptr
        );
        // TODO : here and in every buffer need to count in alignment
        for (const auto& material : pipeline.second.materials) {
            // bind material descs
            vkCmdBindDescriptorSets(
                cmd_buffer_h, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
                1, 1, &material.second.descriptor, 0, nullptr
            );

            for (const auto& mesh_group : material.second.renderables) {
                const auto& vertex_buf = _meshes[mesh_group.first];
                const auto vertex_h = vertex_buf.vertex.handle();

                vkCmdBindVertexBuffers(cmd_buffer_h, 0, 1, &vertex_h, offsets);
                vkCmdBindIndexBuffer(cmd_buffer_h, vertex_buf.index.handle(), 0, VK_INDEX_TYPE_UINT32);

                const u32_t index_count = static_cast<u32_t>(vertex_buf.index.size() / sizeof(u32_t));
                for (const auto& renderable : mesh_group.second) {
                    if (renderable.desc_h != persistent_index_null) {
                        vkCmdBindDescriptorSets(
                            cmd_buffer_h, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
                            2, 1, &pipeline.second.renderable_descs[frame_index][renderable.desc_h], 0, nullptr
                        );
                    }
                    vkCmdDrawIndexed(cmd_buffer_h, index_count, 1, 0, 0, instance_count);
                    instance_count += 1;
                }
            }
        }
    }


    vkCmdEndRenderPass(cmd_buffer_h);
    vkEndCommandBuffer(cmd_buffer_h);
    _swapchain.submit_frame(_present_queue.handle(), frame_index, cmd_buffer_h);
}

VkPhysicalDevice vulkan_renderer::find_physical_device() {
    static constexpr VkQueueFlags device_queue_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;

    auto choose_device_lambda = [this](VkPhysicalDevice device) -> bool {
        return
            vkw::check_device_extension_support(device, _device_extensions) &&
            vkw::check_device_feature_support(device, _device_features) &&
            vkw::check_device_queue_support(device, device_queue_flags) &&
            vkw::check_swap_format_support(device, _surface.handle(), _primary_image_format, _primary_image_colorspace) &&
            vkw::check_swap_present_mode_support(device, _surface.handle(), _primary_image_present_mode);
    };
    const auto phys_devices = vk_instance().enumerate_physical_devices();
    const auto device_it = std::find_if(phys_devices.begin(), phys_devices.end(), choose_device_lambda);
    if (device_it == phys_devices.end()) {
        LOG_ERR("No suitable physical device found to initialize vulkan");
        dbg::panic();
    }

    return *device_it;
}

vulkan_renderer::populated_queue_info vulkan_renderer::populate_queue_infos(VkPhysicalDevice phys_device) {
    u32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, queue_families.data());

    using get_queue_ptr = u32_t(*)(std::span<const VkQueueFamilyProperties>);
    auto find_queue_index = [&queue_families](get_queue_ptr fun, VkQueueFlags flags) {
        u32_t ret_ind = fun(queue_families);
        if (ret_ind == (std::numeric_limits<u32_t>::max)()) {
            ret_ind = vkw::get_any_index(queue_families, flags);
        }
        return ret_ind;
    };

    // NOTE : here is the order, how they will be returned : present, graphics, transfer
    std::array<queue_family_info, 3> family_info_vals{};
    std::vector<queue_family_info*> family_infos{
            &family_info_vals[0],& family_info_vals[1],& family_info_vals[2]
    };
    // present is special, find without lambda
    family_infos[0]->family_ind = vkw::get_present_index(queue_families, phys_device, _surface.handle());
    if (family_infos[0]->family_ind == (std::numeric_limits<u32_t>::max)()) {
        LOG_ERR("Could not find present queue for initializing vulkan");
        dbg::panic();
    }

    family_infos[1]->family_ind = find_queue_index(vkw::get_separate_graphics_index, VK_QUEUE_GRAPHICS_BIT);
    family_infos[2]->family_ind = find_queue_index(vkw::get_separate_transfer_index, VK_QUEUE_TRANSFER_BIT);

    // create queue infos for every family index
    std::vector<vkw::queue_info> queue_infos;
    for (auto i = 0u; i < family_infos.size(); ++i) {
        vkw::queue_info queue_info{
            .queue_family_index = family_infos[i]->family_ind,
            .queue_count = 1
        };

        family_infos[i]->queue_ind = 0;
        // find fam infos with the same queue index, if found merge them and incrememnt queue count
        auto fam_it = family_infos.begin() + i + 1;
        while (true) {
            fam_it = std::find_if(fam_it, family_infos.end(),
                [queue_info](const auto& queue_fam) { return queue_fam->family_ind == queue_info.queue_family_index; }
            );

            if (fam_it == family_infos.end()) {
                break;
            }
            // found same family ind, assign queue index, increment queue count
            if (queue_info.queue_count >= queue_families[(*fam_it)->family_ind].queueCount) {
                LOG_WRN("Exceeded queue capacity of %i for family at index %i, using queue %i as fallback",
                    queue_info.queue_count, (*fam_it)->family_ind, queue_info.queue_count - 1
                );
                (*fam_it)->queue_ind = queue_info.queue_count - 1;
            }
            else {
                (*fam_it)->queue_ind = queue_info.queue_count;
                queue_info.queue_count += 1;
            }
            // erase, by the end all family inds with same value will be merged into 1
            fam_it = family_infos.erase(fam_it);
        }
        // push queue info
        queue_info.priorities.resize(queue_info.queue_count, 1.0f); // NOTE : default to 1
        queue_infos.push_back(std::move(queue_info));
    }
    return { std::move(family_info_vals), std::move(queue_infos) };
}

void vulkan_renderer::create_global_descriptors() {
    constexpr std::array global_layout_bindings{ _camera_data_layout_binding, _model_data_layout_binding };
    _global_descriptor_layout = vkw::vk_descriptor_layout{ _device, global_layout_bindings };
    
    const std::array global_desc_pool_sizes{
        VkDescriptorPoolSize{.type = _camera_data_layout_binding.descriptorType, .descriptorCount = _image_count},
        VkDescriptorPoolSize{.type = _model_data_layout_binding.descriptorType, .descriptorCount = _image_count}
    };
    _global_descriptor_pool = vkw::vk_descriptor_pool{ _device, global_desc_pool_sizes, _image_count };

    _global_descriptors.resize(_image_count);
    _global_descriptor_pool.create_sets(_global_descriptors, _global_descriptor_layout.handle());

    // create model map buffers and camera ubos for each frame and update destriptors
    _model_mat_storage.reserve(_image_count);
    _camera_ubos.reserve(_image_count);
    constexpr VkDeviceSize storage_size = _primary_model_mat_storage_size * sizeof(model_transform);

    std::array global_desc_writes{
        desc_write_from_binding(_camera_data_layout_binding),
        desc_write_from_binding(_model_data_layout_binding)
    };

    for (auto i = 0u; i < _image_count; ++i) {
        _camera_ubos.emplace_back(
            _device, sizeof(camera_transform),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        _model_mat_storage.emplace_back(
            _device, storage_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        VkDescriptorBufferInfo camera_buffer_info{};
        camera_buffer_info.buffer = _camera_ubos.back().handle();
        camera_buffer_info.range = _camera_ubos.back().size();
        camera_buffer_info.offset = 0;

        VkDescriptorBufferInfo model_buffer_info{};
        model_buffer_info.buffer = _model_mat_storage.back().handle();
        model_buffer_info.range = _model_mat_storage.back().size();
        model_buffer_info.offset = 0; // TODO : can have one buffer for all frames, just specify an offset

        global_desc_writes[0].dstSet = _global_descriptors[i];
        global_desc_writes[0].pBufferInfo = &camera_buffer_info;

        global_desc_writes[1].dstSet = _global_descriptors[i];
        global_desc_writes[1].pBufferInfo = &model_buffer_info;

        vkUpdateDescriptorSets(_device.handle(), static_cast<u32_t>(global_desc_writes.size()), global_desc_writes.data(), 0, nullptr);
    }
}

VkDescriptorSet vulkan_renderer::create_material_descriptor(shader_pipeline& pipeline, const material_data& mat_data) {
    // TODO : this has to be enforced in some way, missing can be set to fallback though
    assert(pipeline.shader_data.comb_sampler_infos.size() == mat_data.texs.size());
    std::vector<VkWriteDescriptorSet> desc_writes;
    desc_writes.reserve(mat_data.texs.size());

    VkDescriptorSet desc_set = pipeline.descpool.get_descriptor_set();
    std::vector<VkDescriptorImageInfo> image_infos;
    image_infos.reserve(mat_data.texs.size());

    for (auto i = 0u; i < mat_data.texs.size(); ++i) {
        const auto& sampler_info = pipeline.shader_data.comb_sampler_infos[i];
        auto desc_write = desc_write_from_binding(pipeline.shader_data.layout_bindings[sampler_info.binding_ind]);
        desc_write.dstSet = desc_set;
        
        const auto& sampler = _samplers[mat_data.texs[i]];
        VkDescriptorImageInfo img_info{};
        img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        img_info.imageView = sampler.texture.view().handle();
        img_info.sampler = sampler.sampler.handle();

        image_infos.push_back(std::move(img_info));
        desc_write.pImageInfo = &image_infos.back();

        desc_writes.push_back(std::move(desc_write));
    }

    vkUpdateDescriptorSets(_device.handle(), static_cast<u32_t>(desc_writes.size()), desc_writes.data(), 0, nullptr);
    return desc_set;
}

persistent_index_type vulkan_renderer::create_renderable_frame_descriptors(shader_pipeline& pipeline, std::span<const buffer_hash> buffers) {
    assert(buffers.size() == pipeline.shader_data.buffer_infos.size()); // shouldn't happen, just in case
    std::vector<VkWriteDescriptorSet> desc_writes(buffers.size());
    for (auto i = 0u; i < buffers.size(); ++i) {
        desc_writes[i] = desc_write_from_binding(pipeline.shader_data.layout_bindings[pipeline.shader_data.buffer_infos[i].binding_ind]);
    }

    std::vector<VkDescriptorBufferInfo> buffer_infos(buffers.size());
    for (auto i = 0u; i < buffers.size(); ++i) {
        buffer_infos[i] = pipeline.shader_data.buffer_infos[i].info;
    }

    persistent_index_type ret_ind{};
    for (auto i = 0u; i < _image_count; ++i) {
        VkDescriptorSet desc_set{};
        for (auto j = 0u; j < buffers.size(); ++i) {
            buffer_infos[j].buffer = _buffers[i][buffers[j]].handle();
            desc_writes[j].pBufferInfo = &buffer_infos[j];
            desc_writes[j].dstSet = desc_set;
        }
        vkUpdateDescriptorSets(_device.handle(), static_cast<u32_t>(desc_writes.size()), desc_writes.data(), 0, nullptr);
        ret_ind = pipeline.renderable_descs[i].emplace(desc_set);
    }
    return ret_ind;
}

vulkan_renderer::shader_pipeline vulkan_renderer::create_pipeline(const asset::shader_source& shader) {
    std::vector<vkw::vk_shader_module> shader_modules;
    shader_modules.reserve(shader.oth_stages.size() + 1);
    shader_modules.emplace_back(_device, shader.vert_stage.spirv, asset::shader_vk_stage(shader.vert_stage.stage));
    for (const auto& shader_stage : shader.oth_stages) {
        shader_modules.emplace_back(_device, shader_stage.spirv, asset::shader_vk_stage(shader_stage.stage));
    }

    shader_pipeline new_pipeline;
    const auto capabilities = _device.surface_capabilities(_surface.handle());
    new_pipeline.shader_data = asset::shader_vk_info(shader, { _camera_data_layout_binding , _model_data_layout_binding });

    std::vector<VkDescriptorSetLayout> desc_layouts{ _global_descriptor_layout.handle() };

    if (new_pipeline.shader_data.layout_bindings.size() != 0) {
        new_pipeline.layout = vkw::vk_descriptor_layout{ _device, new_pipeline.shader_data.layout_bindings };
        desc_layouts.push_back(new_pipeline.layout.handle());

        // deduce pool sizes
        std::vector<VkDescriptorPoolSize> pool_sizes;
        if (new_pipeline.shader_data.buffer_infos.size() != 0) {
            pool_sizes.emplace_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _primary_desc_pool_capacity);
        }
        if (new_pipeline.shader_data.comb_sampler_infos.size() != 0) {
            pool_sizes.emplace_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _primary_desc_pool_capacity);
        }
        new_pipeline.descpool = vkw::vk_descriptor_superpool{ _device, pool_sizes, _primary_desc_pool_capacity, new_pipeline.layout.handle() };
    }   

    new_pipeline.pipeline = vkw::vk_pipeline_graphics{
        _device, _render_pass, capabilities.currentExtent,
        shader_modules, new_pipeline.shader_data, desc_layouts
    };

    new_pipeline.renderable_descs.resize(_image_count);
    return new_pipeline;
}

vulkan_renderer::vertex_buffer vulkan_renderer::create_vertex_buffers(const asset::mesh_source& mesh) {
    vertex_buffer buffer;
    buffer.index = _transfer_queue.create_local_buffer(mesh.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    buffer.vertex = _transfer_queue.create_local_buffer(mesh.vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    return buffer;
}

vulkan_renderer::tex_sampler vulkan_renderer::create_sampler(const asset::texture_source& texture) {
    tex_sampler sampler;
    const u32_t mip_levels = static_cast<u32_t>(std::log2((std::max)(texture.width, texture.height)));

    vkw::vk_buffer staging_buffer{ _device,
        texture.pixel_data.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };
    staging_buffer.write(texture.pixel_data);

    sampler.texture = vkw::vk_image_view_pair{ _device,
        VkExtent2D{ texture.width, texture.height },
        mip_levels,
        VK_SAMPLE_COUNT_1_BIT,
        asset::texture_vk_format(texture),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    };

    _graphics_queue.transition_image_layout(sampler.texture.image(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    _transfer_queue.copy_buffer_to_image(staging_buffer.handle(), sampler.texture.image());
    _graphics_queue.generate_mip_maps(sampler.texture.image());

    sampler.sampler = vkw::vk_tex_sampler{ _device, mip_levels };

    return sampler;
}

vulkan_renderer::buffer_hash vulkan_renderer::create_buffer(const asset::vk_shader_data& shader_data, const decltype(shader_data.buffer_infos)::value_type& buffer_info) {
    buffer_hash ret{};
    for (auto& frame_buffers : _buffers) {
        ret = frame_buffers.emplace(_device,
            buffer_info.info.range,
            descriptor_type_to_buffer_usage(shader_data.layout_bindings[buffer_info.binding_ind].descriptorType),
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT // NOTE : writable -> local and coherent
        );
    }
    return ret;
}

const vkw::vk_instance& vulkan_renderer::vk_instance() {
    static constexpr std::array<const char*, 3> instance_extensions{
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME // win only TODO:
    };
    static constexpr std::array<const char*, 1> validation_layers{
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef VKW_ENABLE_DEBUG
    static const vkw::vk_instance main_instance{ instance_extensions, "dry1 instance", validation_layers, vk_debug_callback };
#else
    static const vkw::vk_instance main_instance{ {instance_extensions.data(), 1}, "dry1 instance" };
#endif
    return main_instance;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_renderer::vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data)
{
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        LOG_ERR("\033[32mVALIDATION LAYER\033[0m: %s", data->pMessage);
    }
    return VK_FALSE;
}

}