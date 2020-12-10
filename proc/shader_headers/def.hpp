#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/matrix.hpp>

namespace pcsr::def
{
    struct VertexInput
    {
        glm::vec<3, float> inPosition;
        glm::vec<2, float> inUV;
    };

    struct UniformBufferObject
    {
        alignas(16) glm::mat<4, 4, float> model;
        alignas(16) glm::mat<4, 4, float> view;
        alignas(16) glm::mat<4, 4, float> proj;
    };
}
