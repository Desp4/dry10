#pragma once

#include "vkw/device/device.hpp"

namespace vkw
{
    enum class ShaderType : uint32_t
    {
        Vertex = VK_SHADER_STAGE_VERTEX_BIT,
        Geometry = VK_SHADER_STAGE_GEOMETRY_BIT,
        Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
        Compute = VK_SHADER_STAGE_COMPUTE_BIT
    };

    class ShaderModule : public Movable<ShaderModule>
    {
    public:
        using Movable<ShaderModule>::operator=;

        ShaderModule() = default;
        ShaderModule(ShaderModule&&) = default;
        ShaderModule(const Device* device, std::span<const uint32_t> bin, ShaderType type);
        ~ShaderModule();

        const VkHandle<VkShaderModule>& shaderModule() const { return _module; }
        const ShaderType& type() const { return _type; }

    private:
        DevicePtr _device;

        VkHandle<VkShaderModule> _module;
        ShaderType _type;
    };
}
