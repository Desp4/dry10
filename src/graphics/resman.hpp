#pragma once

#include <unordered_map>
#include <deque>

#include "util/persistent_array.hpp"
#include "util/sparse_set.hpp"

#include "renderer.hpp"
#include "material.hpp"

#include "vkw/buffer.hpp"
#include "vkw/texsampler.hpp"
#include "vkw/desc/descpool.hpp"

#include "asset/meshasset.hpp"

namespace dry::gr {

template<typename T>
struct refcounted_t {
    T value;
    uint32_t count;
};

class resource_manager;

struct renderable {
    friend class resource_manager;
private:
    std::vector<size_t> sampler_ids;
    std::vector<util::size_pt> ubo_ids;
    struct descriptor_id {
        size_t pipeline_id;
        util::size_pt data_id;
    } desc_id;
};

class resource_manager {
public:
    resource_manager(const graphics_instance& instance);

    renderable create_renderable(const material& mat, const asset::mesh_asset& mesh);
    void destroy_renderable(renderable& rend);

    // NOTE : without shader reflection have to know the order of ubos to index them
    void write_to_buffer(const renderable& rend, uint32_t ubo, const void* data, uint32_t size);

    // perhaps doesn't make much sense in contex of it being a resman not a renderer, but it's convenient
    void advance_frame();
    void submit_frame();

private:
    struct mesh_data {
        vkw::buffer_base vertex_buffer;
        vkw::buffer_base index_buffer;
    };
    struct combined_sampler_data {
        vkw::image_view_pair texture;
        vkw::tex_sampler sampler;
    };
    struct persistent_recording_data {
        size_t mesh_id;
        vkw::descriptor_sets descriptor_sets;
    };
    struct pipeline_group {
        vkw::pipeline_graphics pipeline;
        util::sparse_set<persistent_recording_data> recording_data;
        vkw::descriptor_layout layout;
        uint32_t true_size;
    };
    struct expired_renderable {
        persistent_recording_data recording_data;

        std::vector<size_t> sampler_ids;
        std::vector<util::size_pt> ubo_ids;
        size_t pipeline_id;

        uint8_t expired_count;
    };

    constexpr static uint32_t POOL_CAPACITY = 512;
    // TODO : hardcoded part until compile time reflection arrives
    // NOTE : on sizes:
    //      poolSize - max number of this descriptor type used across all allocated descriptors
    //      pool capacity(maxSets) - TOTAL number of sets allocated from a pool
    // excerpt that makes it clear:
    //  "You specify the following parameters during pool creation:
    // 2 sets in total and 2 combined image samplers and 2 uniform buffers.
    // This means that You can allocate 2 descriptor sets where:
    // -both containing 1 combined image sampler and 1 uniform buffer, or
    // -one containing 2 combined image samplers and one 2 uniform buffers, or
    // -one containing 2 combined image samplers and 1 uniform buffer, while the second containing only 1 uniform buffer"
    constexpr static std::array<VkDescriptorPoolSize, 2> POOL_SIZES{
        VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, POOL_CAPACITY },
        VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, POOL_CAPACITY }
    };

    vkw::queue_graphics _graphics_queue;
    vkw::queue_transfer _transfer_queue;

    renderer _renderer;

    frame_context _frame_ctx;

    // meshes, samplers and pipelines in an unurdered map where size_t is a dab asset hash value for fast user level lookup
    // meshes and samplers don't need to be contiguous so can be a persistent array if not for dab hashes
    std::unordered_map<size_t, refcounted_t<mesh_data>> _mesh_buffers;

    // using combined image samplers
    std::unordered_map<size_t, refcounted_t<combined_sampler_data>> _combined_samplers;
    // ubos
    // NOTE : persistent array allocations are deterministic so every single one has the same index for a given ubo
    std::vector<util::persistent_array<vkw::buffer_base>> _ubos;

    std::unordered_map<size_t, pipeline_group> _pipeline_groups;
    std::deque<expired_renderable> _expired_recording_datas;

    // TODO : hardcoded desc types, should be solved on compile time reflection
    // TODO : only one pool, should be dynamically allocated if ran out
    vkw::descriptor_pool _desc_pool;
};

}