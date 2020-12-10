#pragma once

#include "shader/shader.hpp"

#include "shader_headers/pcsrtype.hpp"

namespace vkw
{
    // NOTE : by value because *can* get invalidated in a database, whatever really
    struct Shader
    {
        pcsr::ShaderData data;
        std::vector<VkShaderModule> modules;
        std::vector<VkShaderStageFlagBits> stages;
        VkDescriptorSetLayout layout;
    };

    struct Material
    {
        Shader shader;
        VkBool32 blendEnabled;
        uint32_t matID;
    };
}