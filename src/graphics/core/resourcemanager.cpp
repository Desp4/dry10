#include "resourcemanager.hpp"

#include "dbg/log.hpp"

namespace gr::core
{
    ResourceManager::ResourceManager(const GraphicsInstance& instance) :
        _device(&instance.device()),
        _renderer(&instance),
        _descPool(_device, POOL_SIZES, POOL_CAPACITY)
    {
        auto queueInds = instance.graphicsQueue();
        _graphicsQueue = vkw::GraphicsQueue(_device, queueInds.first, queueInds.second);

        queueInds = instance.transferQueue();
        _transferQueue = vkw::TransferQueue(_device, queueInds.first, queueInds.second);
    }

    RenderableHandle ResourceManager::createRenderable(const Material& material, const asset::MeshAsset& mesh)
    {
        dab::ShaderVkData vkShaderData = material.shader->vkData();
        VkDescriptorSetLayout layout;
        VkHashTable<Renderable>* renderableTable;

        Renderable renderable;

        // check descriptors
        if (!_shaderLookup.contains(material.shader->hash))
        {
            vkw::DescriptorLayout descLayout(_device, vkShaderData.layoutBindings);
            layout = descLayout.layout().handle;

            // create pipeline
            renderableTable = &_renderables.emplace(layout,
                std::pair{ _renderer.createPipeline(material, descLayout), VkHashTable<Renderable>{} })
                .first->second.second;

            _descriptorLayouts.emplace(layout, std::move(descLayout));
            _shaderLookup.emplace(material.shader->hash, layout);
        }
        else
        {
            const VkHash shaderHash = _shaderLookup[material.shader->hash];
            layout = _descriptorLayouts[shaderHash].layout();

            renderableTable = &_renderables[shaderHash].second;
        }

        // check meshes
        if (!_meshLookup.contains(mesh.hash))
        {
            MeshData newMesh;
            newMesh.indexBuffer = _transferQueue.createLocalBuffer(
                _device,
                mesh.indices.size() * sizeof(decltype(mesh.indices)::value_type),
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                mesh.indices.data());
            newMesh.vertexBuffer = _transferQueue.createLocalBuffer(
                _device,
                mesh.vertices.size() * sizeof(decltype(mesh.vertices)::value_type),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                mesh.vertices.data());

            renderable.meshID = _meshBuffers.emplace(newMesh.hash(), std::pair{ 1, std::move(newMesh) })
                .first->first;
            _meshLookup.emplace(mesh.hash, renderable.meshID);
        }
        else
        {
            renderable.meshID = _meshLookup[mesh.hash];
            _meshBuffers[renderable.meshID].first += 1;
        }

        // check textures, texture array order assumed to match that of shader combImageInfos field
        // TODO : if no texture provided and shaders needs one perhaps should use a fallback one
        PANIC_ASSERT(material.textures.size() == vkShaderData.combImageInfos.size(),
            "material texture count does not match that of a shader's: material has %zu, shader has %zu",
            material.textures.size(), vkShaderData.combImageInfos.size());

        for (const auto& texture : material.textures)
        {
            if (!_combinedSamplerLookup.contains(texture->hash))
            {
                CombinedSamplerData combinedSampler;

                const uint32_t mipLevels = std::log2(std::max<uint32_t>(texture->width, texture->height));

                vkw::Buffer stagingBuffer{
                    _device,
                    texture->width * texture->height * texture->channels, // NOTE : assuming 8bit color
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
                stagingBuffer.writeToMemory(texture->pixelData.data(), texture->pixelData.size());

                // NOTE : some hardcode on the usage too
                combinedSampler.texture = vkw::ImageViewPair{
                    _device,
                    VkExtent2D{ texture->width, texture->height },
                    mipLevels,
                    VK_SAMPLE_COUNT_1_BIT,
                    texture->textureFormat(),
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT };

                _graphicsQueue.transitionImageLayout(combinedSampler.texture.image(),
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                _transferQueue.copyBufferToImage(stagingBuffer.buffer(), combinedSampler.texture.image());
                _graphicsQueue.generateMipMaps(combinedSampler.texture.image());

                combinedSampler.sampler = vkw::TexSampler(_device, mipLevels);

                const VkHash samplerHash = combinedSampler.hash();
                renderable.samplerIDs.push_back(samplerHash);

                _combinedSamplers.emplace(samplerHash, std::pair{ 1, std::move(combinedSampler) });
                _combinedSamplerLookup.emplace(texture->hash, samplerHash);
            }
            else
            {
                const VkHash samplerHash = _combinedSamplerLookup[texture->hash];
                _combinedSamplers[samplerHash].first += 1;

                renderable.samplerIDs.push_back(samplerHash);
            }
        }

        // TODO : assume layoutBinding descriptor types are present in a pool, see compile time reflection
        renderable.descriptors = _descPool.createSets(layout, Renderer::IMAGE_COUNT);
        // create ubos for each frame image
        renderable.uboCount = vkShaderData.bufferInfos.size();
        renderable.ubos.resize(renderable.uboCount * Renderer::IMAGE_COUNT);
        for (int i = 0; i < Renderer::IMAGE_COUNT; ++i)
        {
            for (int j = 0; j < renderable.uboCount; ++j)
            {
                renderable.ubos[i * renderable.uboCount + j] = vkw::Buffer{
                    _device,
                    vkShaderData.bufferInfos[j].info.range,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
            }
        }

        // update sets
        std::vector<VkWriteDescriptorSet> descWrites;
        descWrites.reserve(vkShaderData.layoutBindings.size());
        // copy layout binding data
        for (const auto& binding : vkShaderData.layoutBindings)
        {
            VkWriteDescriptorSet descWrite{};
            descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descWrite.dstBinding = binding.binding;
            descWrite.descriptorType = binding.descriptorType;
            descWrite.dstArrayElement = 0; // NOTE : seeting these to 1 and 0
            descWrite.descriptorCount = 1;
            descWrites.push_back(descWrite);
        }

        // set frame inpedendent data for write, now only combined samplers
        for (int i = 0; i < renderable.samplerIDs.size(); ++i)
        {
            const auto& hashedSampler = _combinedSamplers[renderable.samplerIDs[i]].second;
            auto& imageInfoPair = vkShaderData.combImageInfos[i];

            imageInfoPair.info.imageView = hashedSampler.texture.view().imageView();
            imageInfoPair.info.sampler = hashedSampler.sampler.sampler();
            imageInfoPair.info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            descWrites[imageInfoPair.layoutBindingInd].pImageInfo = &imageInfoPair.info;
        }

        for (int i = 0; i < Renderer::IMAGE_COUNT; ++i)
        {
            // cycle through each possible uniform data type and set descWrites pointer for that type
            // needed only for types that have frame dependent data, for now only ubos
            for (int j = 0; j < vkShaderData.bufferInfos.size(); ++j)
            {
                auto& bufferInfoPair = vkShaderData.bufferInfos[j];

                bufferInfoPair.info.buffer = renderable.ubos[i * renderable.uboCount + j].buffer();

                descWrites[bufferInfoPair.layoutBindingInd].pBufferInfo = &bufferInfoPair.info;
            }
            _descPool.updateDescriptorSet(renderable.descriptors[i], descWrites);
        }
        
        const RenderableHandle retHandle = { layout, renderable.hash() };

        renderableTable->emplace(renderable.hash(), std::move(renderable));

        return retHandle;
    }
    // TODO : need to use a cleanup process that waits for all frames to finish and delete after them
    void ResourceManager::destroyRenderable(RenderableHandle handle)
    {
        auto pipelineGroupIt = _renderables.find(handle.first);
        auto renderableIt = pipelineGroupIt->second.second.find(handle.second);

        // remove mesh
        auto meshIt = _meshBuffers.find(renderableIt->second.meshID);
        meshIt->second.first -= 1;
        if (meshIt->second.first == 0)
        {
            // safe to erase right ahead, also O(n), same for other public lookup
            _meshLookup.erase(
                std::find_if(_meshLookup.begin(), _meshLookup.end(),
                    [meshIt](auto&& val) { return val.second == meshIt->first; }));
            _meshBuffers.erase(meshIt);
        }        

        // remove images
        for (VkHash samplerID : renderableIt->second.samplerIDs)
        {
            auto samplerIt = _combinedSamplers.find(samplerID);
            samplerIt->second.first -= 1;
            if (samplerIt->second.first == 0)
            {
                _combinedSamplerLookup.erase(
                    std::find_if(_combinedSamplerLookup.begin(), _combinedSamplerLookup.end(),
                        [samplerIt](auto&& val) { return val.second == samplerIt->first; }));
                _combinedSamplers.erase(samplerIt);
            }
        }

        pipelineGroupIt->second.second.erase(renderableIt);
        // NOTE : if no renderables also delete pipeline, perhaps you wouldn't want to do that
        if (pipelineGroupIt->second.second.size() == 0)
        {
            _shaderLookup.erase(
                std::find_if(_shaderLookup.begin(), _shaderLookup.end(),
                    [handle](auto&& val) { return val.second == handle.first; }));

            _descriptorLayouts.erase(handle.first);
            _renderables.erase(pipelineGroupIt);
        }
    }

    void ResourceManager::writeToUniformBuffer(RenderableHandle handle, uint32_t index, const void* data, uint32_t size)
    {
        Renderable& renderable = _renderables[handle.first].second[handle.second];
        renderable.ubos[_currentFrame.first.frameIndex * renderable.uboCount + index].writeToMemory(data, size);
    }

    void ResourceManager::advanceFrame()
    {
        _currentFrame = _renderer.beginFrame();
    }

    void ResourceManager::submitFrame()
    {
        VkDeviceSize offsets[1]{ 0 };
        const auto& cmdBuffer = _currentFrame.second->buffer();
        const uint32_t frameIndex = _currentFrame.first.frameIndex;

        for (const auto& rendPair : _renderables)
        {
            rendPair.second.first.bindPipeline(cmdBuffer);

            for (const auto& renderable : rendPair.second.second)
            {
                const auto& meshData = _meshBuffers[renderable.second.meshID].second;

                vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &meshData.vertexBuffer.buffer(), offsets);
                vkCmdBindIndexBuffer(cmdBuffer, meshData.indexBuffer.buffer(), 0, VK_INDEX_TYPE_UINT32);
                rendPair.second.first.bindDescriptorSets(cmdBuffer, std::span{ renderable.second.descriptors.data() + frameIndex, 1 });
                vkCmdDrawIndexed(cmdBuffer, meshData.indexBuffer.size() / sizeof(uint32_t), 1, 0, 0, 0);
            }
        }

        _renderer.submitFrame(_currentFrame.first);
    }
}