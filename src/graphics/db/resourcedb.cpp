#include "resourcedb.hpp"

#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "resources.hpp"

namespace gr
{
    std::vector<uint32_t> ResourceDB::dumpShaderBin(const std::string& path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        const uint32_t size = file.tellg();
        std::vector<uint32_t> ret(size / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(ret.data()), size);
        file.close();
        return ret;
    }

    ResourceDB::ResourceDB(const vkw::Device* device, const vkw::GraphicsQueue* grQueue, const vkw::TransferQueue* trQueue) :
        _device(device),
        _graphicsQueue(grQueue),
        _transferQueue(trQueue),
        _shaders(res::_SHADERS.size()),
        _textures(res::_TEXTURES.size()),
        _models(res::_MODELS.size()),
        _materials(res::_MATERIALS.size())
    {
        // shaders
        std::vector<VkDescriptorPoolSize> descPoolSizes;
        for (int i = 0; i < _shaders.size(); ++i)
        {
            auto& sh = _shaders[i];
            sh.name = res::_SHADERS[i].name;
            sh.data = res::_SHADERS[i].data;
            sh.modules[0] = vkw::ShaderModule(
                _device,
                dumpShaderBin(res::_SHADER_DIR + sh.name + ".vert.spv"),
                vkw::ShaderType::Vertex);
            sh.modules[1] = vkw::ShaderModule(
                _device,
                dumpShaderBin(res::_SHADER_DIR + sh.name + ".frag.spv"),
                vkw::ShaderType::Fragment);

            sh.layout = vkw::DescriptorLayout(_device, res::_SHADERS[i].data.layoutBindings);
            for (const auto binding : res::_SHADERS[i].data.layoutBindings)
            {
                auto p = std::find_if(descPoolSizes.begin(), descPoolSizes.end(),
                    [&binding](const VkDescriptorPoolSize& s){ return s.type == binding.descriptorType; });
                if (p == descPoolSizes.end())
                    descPoolSizes.push_back(VkDescriptorPoolSize{ .type = binding.descriptorType });
            }            
        }
        for (auto& descSize : descPoolSizes)
            descSize.descriptorCount = _DESC_CAPACITY_PER_FRAME * core::Renderer::_IMAGE_COUNT;
        _descPool = vkw::DescriptorPool(_device, descPoolSizes, _DESC_POOL_CAPACITY);

        // textures
        for (int i = 0; i < _textures.size(); ++i)
        {
            auto& tex = _textures[i];
            tex.name = res::_TEXTURES[i].name;
            stbi_uc* data = stbi_load(res::_TEXTURES[i].path,
                &tex.width, &tex.height, &tex.channels, STBI_rgb_alpha);
            tex.channels = 4; // NOTE : force
            _textures[i].data.assign(data, data + tex.width * tex.height * tex.channels);
            stbi_image_free(data);
        }
        // models
        for (int i = 0; i < _models.size(); ++i)
        {
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;
            tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, res::_MODELS[i]);

            for (const auto& shape : shapes)
            {
                // uuh uv branching is sub optimal (and potentially unsafe) :)
                const bool hasUV = shape.mesh.indices[0].texcoord_index != -1;
                assert(hasUV);

                for (const auto& index : shape.mesh.indices)
                {
                    _models[i].pos.emplace_back(
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]);
                    _models[i].uv.emplace_back(
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);
                }
                std::vector<pcsr::def::VertexInput> verts;
                std::vector<uint32_t> inds;
                // TODO : FIRST : yes bad remove asap
                res::writeDefVertex(_models[i].pos, _models[i].uv, verts, inds);
                _defVertInput.push_back(std::move(verts));
                _defVertInds.push_back(std::move(inds));

                // dup name check
                std::string name = shape.name;
                int dupCount = 1;
                while (true)
                {
                    const auto p = std::find_if(_models.begin(), _models.begin() + i,
                        [&name](const Model& m) { return m.name == name; });
                    if ((_models.begin() + i) != p)
                        name == shape.name + '(' + std::to_string(dupCount++) + ')';
                    else
                        break;
                }
                _models[i].name = std::move(name);
            }
        } // too many loops yeah sorry, models over
        // materials
        for (int i = 0; i < _materials.size(); ++i)
        {
            auto& mat = _materials[i];
            mat.name = res::_MATERIALS[i].name;
            mat.blendEnabled = res::_MATERIALS[i].blendEnabled;
            for (int p = 0; p < _shaders.size(); ++p)
            {
                if (_shaders[p].name == res::_MATERIALS[i].shaderName)
                {
                    mat.shaderID = p;
                    break;
                }
            }
        }
    }

    vkw::Renderable ResourceDB::createRenderable(uint32_t matID, const char* modName, std::span<const char* const> texNames) const
    {
        vkw::Renderable renderable;

        const auto model = std::find_if(_models.begin(), _models.end(), [&modName](const Model& m){ return m.name == modName; });
        assert(model != _models.end());

        const uint32_t modID = model - _models.begin();
        renderable.staticData.indexBuf = _transferQueue.ptr->createLocalBuffer(
            _defVertInds[modID].size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, _defVertInds[modID].data());
        renderable.staticData.vertexBuf = _transferQueue.ptr->createLocalBuffer(
            _defVertInput[modID].size() * sizeof(pcsr::def::VertexInput), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, _defVertInput[modID].data());

        for (const auto& texName : texNames)
        {
            const auto texture = std::find_if(_textures.begin(), _textures.end(), [&texName](const Texture& t){ return t.name == texName; });
            assert(texture != _textures.end());
            const uint32_t mipLvl = std::log2(std::max<int32_t>(texture->width, texture->height));

            vkw::Buffer stagingBuf(_device, texture->channels * texture->width * texture->height,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            stagingBuf.writeToMemory(texture->data.data(), texture->data.size());

            renderable.staticData.tex = vkw::ImageViewPair(
                _device,
                VkExtent2D{ uint32_t(texture->width), uint32_t(texture->height) },
                mipLvl,
                VK_SAMPLE_COUNT_1_BIT,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT); // TODO : some are hard coded(format for instance) but shouldn't
            _graphicsQueue.ptr->transitionImageLayout(renderable.staticData.tex.image(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            _transferQueue.ptr->copyBufferToImage(stagingBuf.buffer(), renderable.staticData.tex.image());
            _graphicsQueue.ptr->generateMipMaps(renderable.staticData.tex.image());

            renderable.staticData.texSampler = vkw::TexSampler(_device, mipLvl);
        }

        renderable.staticData.texInfo.imageView   = renderable.staticData.tex.view().imageView();
        renderable.staticData.texInfo.sampler     = renderable.staticData.texSampler.sampler();
        renderable.staticData.texInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        assert(matID < _materials.size());

        const Shader& shader = _shaders[_materials[matID].shaderID];

        renderable.frameData.resize(core::Renderer::_IMAGE_COUNT);
        for (auto& frameData : renderable.frameData)
        {
            frameData.ubos.resize(shader.data.bufferInfos.size());
            for (int i = 0; i < frameData.ubos.size(); ++i)
            {
                frameData.ubos[i].ubo = vkw::Buffer(
                    _device,
                    shader.data.bufferInfos[i].range,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                frameData.ubos[i].bufInfo = shader.data.bufferInfos[i];
                frameData.ubos[i].bufInfo.buffer = frameData.ubos[i].ubo.buffer();
            }                
        }

        renderable.descSets = _descPool.createSets(shader.layout.layout(), core::Renderer::_IMAGE_COUNT);
        std::vector<VkWriteDescriptorSet> descWrites(shader.data.layoutBindings.size());
        for (int i = 0; i < descWrites.size(); ++i)
        {
            auto& write = descWrites[i];
            const auto& binding = shader.data.layoutBindings[i];
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstBinding = binding.binding;
            write.descriptorType = binding.descriptorType;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            if (write.descriptorType >= VK_DESCRIPTOR_TYPE_SAMPLER &&
                write.descriptorType <= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                write.pImageInfo = &renderable.staticData.texInfo;
        }

        for (int i = 0; i < renderable.descSets.sets().size(); ++i)
        {
            auto& set = renderable.descSets.sets()[i];
            for (int j = 0; j < descWrites.size(); ++j)
            {
                if (descWrites[j].descriptorType >= VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER &&
                    descWrites[j].descriptorType <= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
                    descWrites[j].pBufferInfo = &renderable.frameData[i].ubos[j].bufInfo;
            }
            _descPool.updateDescriptorSet(set, descWrites);
        }
        return renderable;
    }

    vkw::Material ResourceDB::vkMaterial(const char* matName) const
    {
        const auto material = std::find_if(_materials.begin(), _materials.end(), [&matName](const Material& m) { return m.name == matName; });
        assert(material != _materials.end());

        const Shader& shader = _shaders[material->shaderID];
        vkw::Material ret;
        ret.matID = material - _materials.begin();
        ret.blendEnabled = material->blendEnabled;
        ret.shader.data = shader.data;
        ret.shader.layout = shader.layout.layout();
        ret.shader.modules.resize(2);
        ret.shader.stages.resize(2);
        for (int i = 0; i < 2; ++i)
        {
            ret.shader.modules[i] = shader.modules[i].module();
            ret.shader.stages[i] = static_cast<VkShaderStageFlagBits>(shader.modules[i].type());
        }
        return ret;
    }

    uint32_t ResourceDB::materialID(const char* matName) const
    {
        const auto mat = std::find_if(_materials.begin(), _materials.end(), [&matName](const Material& m) { return m.name == matName; });
        assert(mat != _materials.end());
        return mat - _materials.begin();
    }
}