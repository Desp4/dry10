#include "resourcemanager.hpp"

#include "dbg/log.hpp"

#include <algorithm>

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
        // sort so that ubo indexing is in increasing binding value
        std::sort(vkShaderData.bufferInfos.begin(), vkShaderData.bufferInfos.end(),
            [&bindTable = vkShaderData.layoutBindings](auto&& l, auto&& r)
            {
                return bindTable[l.layoutBindingInd].binding < bindTable[r.layoutBindingInd].binding;
            }
        );

        VkDescriptorSetLayout layout;
        VkHashTable<Renderable>* renderableTable;

        Renderable renderable;

        LOG_DBG("creating renderable[init raw %p;%p]", &material, &mesh);

        // check descriptors
        if (!_shaderLookup.contains(material.shader->hash))
        {
            LOG_DBG("shader[public hash %s] not registered, creating a pipeline", material.shader->hash.c_str());

            vkw::DescriptorLayout descLayout(_device, vkShaderData.layoutBindings);
            layout = descLayout.layout().handle;

            // create pipeline
            renderableTable = &_renderables.emplace(layout,
                RenderableGroup{ _renderer.createPipeline(material, descLayout), VkHashTable<Renderable>{}, 1 })
                .first->second.renderables;

            _descriptorLayouts.emplace(layout, std::move(descLayout));
            _shaderLookup.emplace(material.shader->hash, layout);
        }
        else
        {
            LOG_DBG("shader[public hash %s] registered, binding object", material.shader->hash.c_str());
            
            const VkHash shaderHash = _shaderLookup[material.shader->hash];
            layout = _descriptorLayouts[shaderHash].layout();

            auto& renderableGroup = _renderables[shaderHash];
            renderableGroup.trueSize += 1;
            renderableTable = &renderableGroup.renderables;
        }

        // check meshes
        if (!_meshLookup.contains(mesh.hash))
        {
            LOG_DBG("mesh[public hash %s] not registered, allocating buffers", mesh.hash.c_str());

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
            LOG_DBG("mesh[public hash %s] registered, binding object", mesh.hash.c_str());

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
                LOG_DBG("texture[public hash %s] not registered, allocating combined sampler", texture->hash.c_str());

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
                LOG_DBG("texture[public hash %s] registered, binding object", texture->hash.c_str());

                const VkHash samplerHash = _combinedSamplerLookup[texture->hash];
                _combinedSamplers[samplerHash].first += 1;

                renderable.samplerIDs.push_back(samplerHash);
            }
        }

        // TODO : assume layoutBinding descriptor types are present in a pool, see compile time reflection
        renderable.descriptors = _descPool.createSets(layout, _renderer.imageCount());
        // create ubos for each frame image
        renderable.uboCount = vkShaderData.bufferInfos.size();
        renderable.ubos.resize(renderable.uboCount * _renderer.imageCount());
        for (int i = 0; i < _renderer.imageCount(); ++i)
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

        for (int i = 0; i < _renderer.imageCount(); ++i)
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
        LOG_DBG("renderable[handle %p;%p] created", retHandle.first, retHandle.second);

        return retHandle;
    }

    void ResourceManager::destroyRenderable(RenderableHandle handle)
    {
        auto pipelineGroupIt = _renderables.find(handle.first);
        auto renderableIt = pipelineGroupIt->second.renderables.find(handle.second);

        _deletedRenderables.emplace_back(std::move(renderableIt->second), handle.first, 1);
        pipelineGroupIt->second.renderables.erase(renderableIt);

        LOG_DBG("renderable[handle %p;%p] deleted", handle.first, handle.second);
    }

    void ResourceManager::writeToUniformBuffer(RenderableHandle handle, uint32_t index, const void* data, uint32_t size)
    {
        Renderable& renderable = _renderables[handle.first].renderables[handle.second];
        renderable.ubos[_currentFrameCtx.frameIndex * renderable.uboCount + index].writeToMemory(data, size);
    }

    void ResourceManager::advanceFrame()
    {
        _currentFrameCtx = _renderer.beginFrame();

        // update deleted resources, in this place can't be async due to races in logic stage
        // NOTE : append and delete not ideal in a vector, maybe a ringbuffer or some other FIFO structure
        for (auto p = _deletedRenderables.begin(); p != _deletedRenderables.end();)
        {
            p->expireCount += 1;
            if (p->expireCount != _renderer.imageCount())
            {
                ++p;
            }
            else
            {
                LOG_DBG("renderable[handle %p;%p] freed", p->parentGroupHash, p->renderable.hash());

                auto parentGroup = _renderables.find(p->parentGroupHash);
                parentGroup->second.trueSize -= 1;

                // check if need to unload resources
                // meshes
                auto meshIt = _meshBuffers.find(p->renderable.meshID);
                meshIt->second.first -= 1;
                if (meshIt->second.first == 0)
                {
                    LOG_DBG("mesh[hash %p] unused, freeing", p->renderable.meshID);
                    // safe to erase right ahead, also O(n), same for other public lookup
                    _meshLookup.erase(
                        std::find_if(_meshLookup.begin(), _meshLookup.end(),
                            [meshIt](auto&& val) { return val.second == meshIt->first; }));
                    _meshBuffers.erase(meshIt);
                }

                // images
                for (VkHash samplerID : p->renderable.samplerIDs)
                {
                    auto samplerIt = _combinedSamplers.find(samplerID);
                    samplerIt->second.first -= 1;
                    if (samplerIt->second.first == 0)
                    {
                        LOG_DBG("texture[hash %p] unused, freeing", samplerID);

                        _combinedSamplerLookup.erase(
                            std::find_if(_combinedSamplerLookup.begin(), _combinedSamplerLookup.end(),
                                [samplerIt](auto&& val) { return val.second == samplerIt->first; }));
                        _combinedSamplers.erase(samplerIt);
                    }
                }

                // pipeline
                if (parentGroup->second.trueSize == 0)
                {
                    LOG_DBG("pipeline[hash %p] unused, destroying", p->parentGroupHash);
                    _shaderLookup.erase(
                        std::find_if(_shaderLookup.begin(), _shaderLookup.end(),
                            [handle = p->parentGroupHash](auto&& val) { return val.second == handle; }));

                    _descriptorLayouts.erase(p->parentGroupHash);
                    _renderables.erase(parentGroup);
                }

                p = _deletedRenderables.erase(p);
            }
        }
    }

    void ResourceManager::submitFrame()
    {
        VkDeviceSize offsets[1]{ 0 };
        const auto& cmdBuffer = _currentFrameCtx.cmdBuffer->buffer();

        for (const auto& rendPair : _renderables)
        {
            rendPair.second.pipeline.bindPipeline(cmdBuffer);

            for (const auto& renderable : rendPair.second.renderables)
            {
                const auto& meshData = _meshBuffers[renderable.second.meshID].second;

                vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &meshData.vertexBuffer.buffer(), offsets);
                vkCmdBindIndexBuffer(cmdBuffer, meshData.indexBuffer.buffer(), 0, VK_INDEX_TYPE_UINT32);
                rendPair.second.pipeline.bindDescriptorSets(
                    cmdBuffer, std::span{ renderable.second.descriptors.data() + _currentFrameCtx.frameIndex, 1 });
                vkCmdDrawIndexed(cmdBuffer, meshData.indexBuffer.size() / sizeof(uint32_t), 1, 0, 0, 0);
            }
        }

        _renderer.submitFrame(_currentFrameCtx.frameIndex);
    }
}