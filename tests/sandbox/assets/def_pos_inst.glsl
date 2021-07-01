#pragma vertex
#version 460

// === unshaded pass data ===
layout(set = 0, binding = 0) uniform CameraData {
    mat4 viewproj;
    mat4 view;
    mat4 proj;
} cameraData;

struct InstanceData {
    mat4 model;
    uint material;
};

layout(std140, set = 0, binding = 1) readonly buffer InstanceBuffer {
    InstanceData transforms[];
} instanceTransforms;

// === forced vertex input ===
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

// === shader ===
layout(location = 0) out vec3 vertNormal;

void main() {
    mat4 transformMatrix = (cameraData.viewproj * instanceTransforms.transforms[gl_InstanceIndex].model);
    gl_Position = transformMatrix * vec4(inPosition, 1.0);

    vertNormal = inNormal;
}

#pragma fragment
#version 460

layout(location = 0) in vec3 vertNormal;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(vertNormal, 1.0);
}