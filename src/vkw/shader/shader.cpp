#include "shader.hpp"

namespace vkw
{
    ShaderModule::ShaderModule(const Device* device, std::span<const uint32_t> bin, ShaderType type) :
        _device(device),
        _type(type)
    {
        VkShaderModuleCreateInfo shaderInfo{};
        shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderInfo.codeSize = bin.size() * sizeof(uint32_t);
        shaderInfo.pCode = bin.data();

        vkCreateShaderModule(_device->device(), &shaderInfo, NULL_ALLOC, &_module);
    }

    ShaderModule::~ShaderModule()
    {
        if (_device) vkDestroyShaderModule(_device->device(), _module, NULL_ALLOC);
    }
}