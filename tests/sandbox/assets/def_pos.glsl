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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 vertNormal;

void main()
{
    mat4 modelMatrix = modelBuffer.objects[gl_BaseInstance].model;
    mat4 transformMatrix = (cameraData.viewproj * modelMatrix);
    gl_Position = transformMatrix * vec4(inPosition, 1.0);

    vertNormal = inNormal;
}

#pragma fragment
#version 460

layout(location = 0) in vec3 vertNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(vertNormal, 1.0);
}