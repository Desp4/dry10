#pragma once

#include <vector>
#include <unordered_map>
#include <utility>

#include "renderer.hpp"

#include "vkw/buffer.hpp"
#include "vkw/texsampler.hpp"
#include "vkw/pipeline_g.hpp"
#include "vkw/desc/descpool.hpp"
#include "vkw/image/imageviewpair.hpp"

#include "asset/meshasset.hpp"

namespace gr::core
{
    using PtrHash = const void*;
    using VkHash = PtrHash;
    // pipeline, renderable
    using RenderableHandle = std::pair<VkHash, VkHash>;

    static constexpr void* NullVkHash = nullptr;

    class ResourceManager
    {
    public:
        ResourceManager(const GraphicsInstance& instance);

        // TODO : assets have path hashes, tmp runtime assets should generate their own pseudo path hashes
        RenderableHandle createRenderable(const Material& material, const asset::MeshAsset& mesh);
        void destroyRenderable(RenderableHandle handle);

        // NOTE : without shader reflection have to know the order of ubos to index them
        void writeToUniformBuffer(RenderableHandle handle, uint32_t index, const void* data, uint32_t size);

        // perhaps doesn't make much sense in contex of it being a resman not a renderer, but it's convenient
        void advanceFrame();
        void submitFrame();

        void waitOnDevice() const { vkDeviceWaitIdle(_device->device()); }

    private:
        template<typename T>
        using VkHashTable = std::unordered_map<VkHash, T>;

        struct Renderable
        {
            // NOTE : doing only combined samplers for now
            std::vector<VkHash> samplerIDs;
            // TODO : can be arrays, should be known at compile time after got the shader data
            // 2d ubo array like this: ubo1_frame1, ubo2_frame1 ..., ubo1_frame2 ...
            std::vector<vkw::Buffer> ubos;
            vkw::DescriptorSets descriptors;
            // number of ubos per frame
            uint32_t uboCount;
            VkHash meshID;

            VkHash hash() const
            {
                return descriptors[0];
            }
        };
        struct MeshData
        {
            vkw::Buffer vertexBuffer;
            vkw::Buffer indexBuffer;

            VkHash hash() const
            {
                return vertexBuffer.buffer().handle;
            }
        };
        struct CombinedSamplerData
        {
            vkw::ImageViewPair texture;
            vkw::TexSampler sampler;

            VkHash hash() const
            {
                return sampler.sampler().handle;
            }
        };
        struct RenderableGroup
        {
            vkw::GraphicsPipeline pipeline;
            VkHashTable<Renderable> renderables;
            uint32_t trueSize;
        };
        struct DeletedRenderable
        {
            Renderable renderable;
            VkHash parentGroupHash;
            uint8_t expireCount;
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

        const vkw::Device* _device;
        vkw::GraphicsQueue _graphicsQueue;
        vkw::TransferQueue _transferQueue;

        Renderer _renderer;

        RenderContext _currentFrameCtx;

        // TODO : hashing vk internal pointers => all unique
        // TODO : TODO : use a hash table for unique val lookup which doesn't construct the structure
        // resource buffers pairs: int - usage counter, vk struct - data
        VkHashTable<std::pair<uint32_t, MeshData>> _meshBuffers;

        // shader resources
        // TODO : using presence of a layout as a flag for a shader(material) existring
        // - not ideal when introduced actual material properties, also wastes layouts for shaders with identical layouts
        // basically: pipeline and descriptor tables have the same size, use the same key(layout)
        VkHashTable<vkw::DescriptorLayout> _descriptorLayouts;

        // using combined image samplers
        VkHashTable<std::pair<uint32_t, CombinedSamplerData>> _combinedSamplers;

        // vkHash is the same as in shaderLookup
        VkHashTable<RenderableGroup> _renderables;
        // renderable deletion queue, NOTE : just a vector for now
        std::vector<DeletedRenderable> _deletedRenderables;

        // hash tables for public access
        // when deleting entries have to manually erase a lookup value by searching through those
        // can introduce an inverse lookup table for these cases if needed
        std::unordered_map<std::string, VkHash> _shaderLookup;
        std::unordered_map<std::string, VkHash> _meshLookup;
        std::unordered_map<std::string, VkHash> _combinedSamplerLookup;

        // TODO : hardcoded desc types, should be solved on compile time reflection
        // TODO : only one pool, should be dynamically allocated if ran out
        vkw::DescriptorPool _descPool;
    };
}