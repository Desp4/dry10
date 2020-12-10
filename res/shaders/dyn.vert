#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out mat4 view;

void main()
{
    vec3 pos = inPosition;
    pos.x = pos.x + sin(ubo.view[3][0]) * sin(pos.x) * 0.25;
    pos.y = pos.y + sin(ubo.view[3][1]) * sin(pos.y) * 0.25;
    pos.z = pos.z + sin(ubo.view[3][2]) * sin(pos.z) * 0.25;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
    fragUV = inUV;
    view = ubo.view;
}
