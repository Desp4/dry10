#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "buffer.hpp"
#include "cmd/cmdbuffer.hpp"
#include "image/imageviewpair.hpp"
#include "texsampler.hpp"

namespace vkw
{
    // FUTURE : SoA for static and frame should probably be better
    struct Renderable
    {
        struct FrameData
        {
            struct UBOBuffer
            {
                VkDescriptorBufferInfo bufInfo;
                Buffer ubo;
            };

            std::vector<UBOBuffer> ubos;
        };

        struct StaticData
        {
            Buffer vertexBuf;
            Buffer indexBuf;

            ImageViewPair tex;
            TexSampler texSampler;
            VkDescriptorImageInfo texInfo;
        };

        StaticData staticData;
        std::vector<FrameData> frameData;
        DescriptorSets descSets;
    };
}