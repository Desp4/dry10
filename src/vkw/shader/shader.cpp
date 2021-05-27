#include "shader.hpp"

namespace dry::vkw {

vk_shader_module::vk_shader_module(const vk_device& device, const_byte_span bin, VkShaderStageFlagBits type) :
    _device{ &device },
    _type{ type }
{
    VkShaderModuleCreateInfo shader_info{};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = bin.size();
    shader_info.pCode = reinterpret_cast<const u32_t*>(bin.data());

    vkCreateShaderModule(_device->handle(), &shader_info, null_alloc, &_module);
}

vk_shader_module::~vk_shader_module() {
    if (_device != nullptr) {
        vkDestroyShaderModule(_device->handle(), _module, null_alloc);
    }   
}

vk_shader_module& vk_shader_module::operator=(vk_shader_module&& oth) {
    // destroy
    if (_device != nullptr) {
        vkDestroyShaderModule(_device->handle(), _module, null_alloc);
    }
    // move
    _device = oth._device;
    _module = oth._module;
    _type = oth._type;
    // null
    oth._device = nullptr;
    return *this;
}

}
