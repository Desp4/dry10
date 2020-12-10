#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragUV;
layout(location = 1) in mat4 view;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main()
{
    outColor = texture(texSampler, fragUV);
    outColor.x = outColor.x + sin(view[3][0]) * sin(outColor.y);
    outColor.y = outColor.y + sin(view[3][1]) * sin(outColor.z);
    outColor.z = outColor.z + sin(view[3][2]) * sin(outColor.x);
}
