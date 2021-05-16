#include "resman.hpp"

#include "asset/vk_reflect.hpp"
#include "dbg/log.hpp"

namespace dry::gr {

resource_manager::resource_manager(const graphics_instance& instance) :
    _renderer(instance),
    _ubos(_renderer.image_count())
{
    queue_ind_info queue_data = instance.graphics_queue();
    _graphics_queue = vkw::vk_queue_graphics(queue_data.family_ind, queue_data.queue_ind);

    queue_data = instance.transfer_queue();
    _transfer_queue = vkw::vk_queue_transfer(queue_data.family_ind, queue_data.queue_ind);
}

renderable resource_manager::create_renderable(const material& mat, const asset::mesh_asset& mesh) {
    asset::vk_shader_data vk_data = asset::shader_vk_info(*mat.shader);
    // sort so that ubo bindings are in increasing order
    std::sort(vk_data.buffer_infos.begin(), vk_data.buffer_infos.end(),
        [&bind_table = vk_data.layout_bindings](auto&& l, auto&& r) {
            return bind_table[l.binding_ind].binding < bind_table[r.binding_ind].binding;
        }
    );

    renderable new_renderable;
    persistent_recording_data new_recording_data;

    pipeline_group* curr_pipeline = nullptr;
    LOG_DBG("creating renderable[0x%08x; 0x%08x]", &mat, &mesh);

    // check descriptors
    if (!_pipeline_groups.contains(mat.shader->hash)) {
        LOG_DBG("shader[hash 0x%08x] not registered, creating a pipeline", mat.shader->hash);

        struct pipeline_group new_pipeline;
        new_pipeline.layout = vkw::vk_descriptor_layout(vk_data.layout_bindings);
        new_pipeline.true_size = 1;
        new_pipeline.pipeline = _renderer.create_pipeline(mat, new_pipeline.layout);
        new_pipeline.textured_desc_pool = descriptor_pool_pool{ POOL_SIZES, POOL_CAPACITY, new_pipeline.layout.handle() };

        curr_pipeline = &_pipeline_groups.emplace(mat.shader->hash, std::move(new_pipeline)).first->second;
    }
    else {
        LOG_DBG("shader[asset 0x%08x] present, binding object", mat.shader->hash);
        curr_pipeline = &_pipeline_groups[mat.shader->hash];
        curr_pipeline->true_size += 1;
    }
    new_renderable.desc_id.pipeline_id = mat.shader->hash;

    // check meshes
    if (!_mesh_buffers.contains(mesh.hash)) {
        LOG_DBG("mesh[hash 0x%08x] not registered, allocating buffers", mesh.hash);

        mesh_data new_mesh;
        new_mesh.index_buffer = _transfer_queue.create_local_buffer(
            mesh.indices,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        );
        new_mesh.vertex_buffer = _transfer_queue.create_local_buffer(
            mesh.vertices,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        );

        _mesh_buffers.emplace(mesh.hash, refcounted_t<mesh_data>{std::move(new_mesh), 1});
    }
    else {
        LOG_DBG("mesh[hash 0x%08x] registered, binding object", mesh.hash);
        _mesh_buffers[mesh.hash].count += 1;
    }
    new_recording_data.mesh_id = mesh.hash;

    // check textures, texture array order assumed to match that of shader combImageInfos field
    // TODO : if no texture provided and shader needs one should use a fallback
    if (mat.textures.size() != vk_data.comb_sampler_infos.size()) {
        LOG_ERR("material texture count does not match that of a shader's: material has %zu, shader has %zu",
            mat.textures.size(), vk_data.comb_sampler_infos.size());
        dbg::panic();
    }

    new_renderable.sampler_ids.reserve(mat.textures.size());
    for (const auto& texture : mat.textures) {
        if (!_combined_samplers.contains(texture->hash)) {
            LOG_DBG("texture[hash 0x%08x] not registered, allocating combined sampler", texture->hash);

            combined_sampler_data new_comb_sampler;
            const uint32_t mip_levels = static_cast<uint32_t>(std::log2((std::max)(texture->width, texture->height)));

            vkw::vk_buffer staging_buffer(
                texture->width * texture->height * texture->channels, // NOTE : assuming 8bit color
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            staging_buffer.write(texture->pixel_data.data(), texture->pixel_data.size());

            // NOTE : some hardcode on the usage too
            new_comb_sampler.texture = vkw::vk_image_view_pair(
                VkExtent2D{ texture->width, texture->height },
                mip_levels,
                VK_SAMPLE_COUNT_1_BIT,
                asset::texture_vk_format(*texture),
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );

            _graphics_queue.transition_image_layout(new_comb_sampler.texture.image(),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            );
            _transfer_queue.copy_buffer_to_image(staging_buffer.handle(), new_comb_sampler.texture.image());
            _graphics_queue.generate_mip_maps(new_comb_sampler.texture.image());

            new_comb_sampler.sampler = vkw::vk_tex_sampler(mip_levels);
            _combined_samplers.emplace(texture->hash, refcounted_t<combined_sampler_data>{std::move(new_comb_sampler), 1});
        }
        else {
            LOG_DBG("texture[hash 0x%08x] registered, binding object", texture->hash);
            _combined_samplers[texture->hash].count += 1;
        }
        new_renderable.sampler_ids.push_back(texture->hash);
    }

    // TODO : assume layoutBinding descriptor types are present in a pool, see compile time reflection
    new_recording_data.descriptor_sets.resize(_renderer.image_count());
    for (auto& desc_set : new_recording_data.descriptor_sets) {
        desc_set = curr_pipeline->textured_desc_pool.get_descriptor_set();
    }

    // create ubos for each frame image
    new_renderable.ubo_ids.reserve(vk_data.buffer_infos.size());
    for (const auto& ubo_info : vk_data.buffer_infos) {
        util::size_pt ubo_ind = util::size_pt_null;
        for (auto& frame_ubos : _ubos) {
            // NOTE : for every frame_ubos ind will be the same, be careful if changing persistent array
            ubo_ind = frame_ubos.emplace(
                ubo_info.info.range,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }
        new_renderable.ubo_ids.push_back(ubo_ind);
    }

    // update sets
    std::vector<VkWriteDescriptorSet> desc_writes;
    desc_writes.reserve(vk_data.layout_bindings.size());
    // copy layout binding data
    for (const auto& binding : vk_data.layout_bindings) {
        VkWriteDescriptorSet desc_write{};
        desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_write.dstBinding = binding.binding;
        desc_write.descriptorType = binding.descriptorType;
        desc_write.dstArrayElement = 0; // NOTE : seeting these to 1 and 0
        desc_write.descriptorCount = 1;
        desc_writes.push_back(desc_write);
    }

    // set frame inpedendent data for write, now only combined samplers
    for (auto i = 0u; i < vk_data.comb_sampler_infos.size(); ++i) {
        VkDescriptorImageInfo& img_info = vk_data.comb_sampler_infos[i].info;
        const combined_sampler_data& sampler_data = _combined_samplers[new_renderable.sampler_ids[i]].value;

        img_info.imageView = sampler_data.texture.view().handle();
        img_info.sampler = sampler_data.sampler.handle();
        img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        desc_writes[vk_data.comb_sampler_infos[i].binding_ind].pImageInfo = &img_info;
    }

    for (auto i = 0u; i < _renderer.image_count(); ++i) {
        // cycle through each possible uniform data type and set desc_writes pointer for that type
        // needed only for types that have frame dependent data, for now only ubos
        for (auto j = 0u; j < vk_data.buffer_infos.size(); ++j) {
            VkDescriptorBufferInfo& buf_info = vk_data.buffer_infos[j].info;
            buf_info.buffer = _ubos[i][new_renderable.ubo_ids[j]].handle();

            desc_writes[vk_data.buffer_infos[j].binding_ind].pBufferInfo = &buf_info;
        }
        curr_pipeline->textured_desc_pool.update_descriptor_set(new_recording_data.descriptor_sets[i], desc_writes);
    }

    new_renderable.desc_id.data_id = curr_pipeline->recording_data.emplace(std::move(new_recording_data));

    LOG_DBG("renderable[0x%08x; 0x%08x] created", new_renderable.desc_id.pipeline_id, new_renderable.desc_id.data_id);
    return new_renderable;
}

void resource_manager::destroy_renderable(renderable& rend) {
    pipeline_group& rend_group = _pipeline_groups[rend.desc_id.pipeline_id];

    expired_renderable expired_rend;
    expired_rend.sampler_ids = std::move(rend.sampler_ids);
    expired_rend.ubo_ids = std::move(rend.ubo_ids);
    expired_rend.recording_data = std::move(rend_group.recording_data[rend.desc_id.data_id]);
    expired_rend.pipeline_id = rend.desc_id.pipeline_id;
    expired_rend.expired_count = 1;

    rend_group.recording_data.remove(rend.desc_id.data_id);
    _expired_recording_datas.emplace_back(std::move(expired_rend));
    LOG_DBG("renderable[0x%08x; 0x%08x] deleted", rend.desc_id.pipeline_id, rend.desc_id.data_id);
}

void resource_manager::write_to_buffer(const renderable& rend, uint32_t ubo, const void* data, uint32_t size) {
    _ubos[_frame_ctx.frame_index][rend.ubo_ids[ubo]].write(data, size);
}

void resource_manager::advance_frame() {
    _frame_ctx = _renderer.begin_frame();

    // update deleted resources, in this place can't be async due to races in logic stage
    // NOTE : append and delete not ideal in a vector, maybe a ringbuffer or some other FIFO structure
    for (auto p = _expired_recording_datas.begin(); p != _expired_recording_datas.end();) {
        p->expired_count += 1;

        if (p->expired_count != _renderer.image_count()) {
            ++p;
        }
        else {
            auto rend_group_it = _pipeline_groups.find(p->pipeline_id);

            // return descriptor sets
            for (const auto desc_set : p->recording_data.descriptor_sets) {
                rend_group_it->second.textured_desc_pool.return_descriptor_set(desc_set);
            }

            // destroy buffers
            for (auto& frame_ubos : _ubos) {
                for (const util::size_pt ubo_id : p->ubo_ids) {
                    frame_ubos.remove(ubo_id);
                }
            }

            // check if need to unload resources
            // meshes
            auto rend_mesh_it = _mesh_buffers.find(p->recording_data.mesh_id);
            rend_mesh_it->second.count -= 1;

            if (rend_mesh_it->second.count == 0) {
                LOG_DBG("mesh[hash 0x%08x] unused, freeing", p->recording_data.mesh_id);
                _mesh_buffers.erase(rend_mesh_it);
            }

            // images
            for (size_t sampler_id : p->sampler_ids) {
                auto rend_sampler_it = _combined_samplers.find(sampler_id);
                rend_sampler_it->second.count -= 1;

                if (rend_sampler_it->second.count == 0) {
                    LOG_DBG("texture[hash 0x%08x] unused, freeing", sampler_id);
                    _combined_samplers.erase(rend_sampler_it);
                }
            }

            // pipeline
            rend_group_it->second.true_size -= 1;

            if (rend_group_it->second.true_size == 0) {
                LOG_DBG("pipeline[hash 0x%08x] unused, destroying", p->pipeline_id);
                _pipeline_groups.erase(rend_group_it);
            }

            LOG_DBG("expired renderable[pipeline 0x%08x] freed", p->pipeline_id);
            // NOTE : expired elements are all in a queue in a strictly decreasing expired_count order
            // that means that if removal happens it happens in the front
            _expired_recording_datas.pop_front();
            p = _expired_recording_datas.begin();
        }
    }
}

void resource_manager::submit_frame() {
    VkDeviceSize offsets[1]{ 0 };
    const VkCommandBuffer& cmd_buf = _frame_ctx.cmd_buf->handle();

    for (const auto& pipeline_pair : _pipeline_groups) {
        const pipeline_group& pipeline_group = pipeline_pair.second;

        pipeline_group.pipeline.bind_pipeline(cmd_buf);
        for (const persistent_recording_data& record_data : pipeline_group.recording_data) {
            const mesh_data& record_mesh = _mesh_buffers[record_data.mesh_id].value;

            auto vert_buffer = record_mesh.vertex_buffer.handle();
            vkCmdBindVertexBuffers(cmd_buf, 0, 1, &vert_buffer, offsets);
            vkCmdBindIndexBuffer(cmd_buf, record_mesh.index_buffer.handle(), 0, VK_INDEX_TYPE_UINT32);
            pipeline_group.pipeline.bind_descriptor_sets(
                cmd_buf, std::span{ record_data.descriptor_sets.data() + _frame_ctx.frame_index, 1 }
            );
            vkCmdDrawIndexed(cmd_buf, static_cast<uint32_t>(record_mesh.index_buffer.size() / sizeof(uint32_t)), 1, 0, 0, 0);
        }
    }
    _renderer.submit_frame(_frame_ctx);
}

}