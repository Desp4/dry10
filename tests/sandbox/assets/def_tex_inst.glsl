#pragma vertex
#version 460

layout(set = 0, binding = 0) uniform CameraData
{
    mat4 viewproj;
    mat4 view;
    mat4 proj;
} cameraData;

struct ModelData {
    mat4 model;
};

layout(std140, set = 0, binding = 1) readonly buffer ModelBuffer {
    ModelData objects[];
} modelBuffer;

// Vertex input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

// Instance input
layout(location = 3) in vec3 instancePos;
layout(location = 4) in vec3 instanceScale;
layout(location = 5) in mat4 instanceRot;
layout(location = 9) in int instanceTexIndex;

layout(location = 0) out vec2 fragUV;

void main()
{
    mat4 modelMatrix = modelBuffer.objects[gl_BaseInstance].model;
    mat4 transformMatrix = (cameraData.viewproj * modelMatrix);
    gl_Position = transformMatrix * vec4(inPosition, 1.0);
    fragUV = inUV;
}

#pragma fragment
#version 460

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main()
{
    outColor = texture(texSampler, fragUV);
}