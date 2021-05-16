#include "shader.hpp"

#include "vkw/device/g_device.hpp"

namespace dry::vkw {

vk_shader_module::vk_shader_module(const_byte_span bin, VkShaderStageFlagBits type) :
    _type{ type }
{
    VkShaderModuleCreateInfo shader_info{};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = bin.size();
    shader_info.pCode = reinterpret_cast<const u32_t*>(bin.data());

    vkCreateShaderModule(g_device->handle(), &shader_info, null_alloc, &_module);
}

vk_shader_module::~vk_shader_module() {
    vkDestroyShaderModule(g_device->handle(), _module, null_alloc);
}

vk_shader_module& vk_shader_module::operator=(vk_shader_module&& oth) {
    // destroy
    vkDestroyShaderModule(g_device->handle(), _module, null_alloc);
    // move
    _module = oth._module;
    // null
    oth._module = VK_NULL_HANDLE;
    return *this;
}

}
