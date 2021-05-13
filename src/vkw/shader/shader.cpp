#include "shader.hpp"
#include "vkw/device/device.hpp"

namespace dry::vkw {

shader_module::shader_module(std::span<const std::byte> bin, shader_type type) :
    _type(type)
{
    VkShaderModuleCreateInfo shader_info{};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = bin.size();
    shader_info.pCode = reinterpret_cast<const uint32_t*>(bin.data());

    vkCreateShaderModule(device_main::device(), &shader_info, NULL_ALLOC, &_module);
}

shader_module::~shader_module() {
    vkDestroyShaderModule(device_main::device(), _module, NULL_ALLOC);
}

}