#pragma vertex
#version 460

layout(set = 0, binding = 0) uniform CameraData
{
    mat4 viewproj;
    mat4 view;
    mat4 proj;
} cameraData;

// Vertex input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

// Instance input
layout(location = 3) in vec4 instanceModel_0;
layout(location = 4) in vec4 instanceModel_1;
layout(location = 5) in vec4 instanceModel_2;
layout(location = 6) in vec4 instanceModel_3;
layout(location = 7) in int instanceTexIndex;

layout(location = 0) out vec3 vertNormal;

void main()
{
    mat4 instanceModel = mat4(instanceModel_0, instanceModel_1, instanceModel_2, instanceModel_3);
    mat4 transformMatrix = (cameraData.viewproj * instanceModel);
    gl_Position = transformMatrix * vec4(inPosition, 1.0);

    vertNormal = inNormal;
}

#pragma fragment
#version 460

#define TEX_ARRAY_SIZE 4096

layout(set = 0, binding = 1) uniform sampler texSampler;
layout(set = 0, binding = 2) uniform texture2D textures[TEX_ARRAY_SIZE];

layout(location = 0) in vec3 vertNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(vertNormal, 1.0);
}