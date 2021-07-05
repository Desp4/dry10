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
struct MaterialData {
    uint texIndex;
};

layout(std140, set = 2, binding = 0) readonly buffer MaterialBuffer {
    MaterialData materials[];
} instanceMaterials;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out uint texIndex;

void main() {
    InstanceData instance = instanceTransforms.transforms[gl_InstanceIndex];

    mat4 transformMatrix = (cameraData.viewproj * instance.model);
    gl_Position = transformMatrix * vec4(inPosition, 1.0);

    fragUV = inUV;
    fragPos = vec3(instance.model * vec4(inPosition, 1.0));
    fragNormal = inNormal;
    texIndex = instanceMaterials.materials[instance.material].texIndex;
}

#pragma fragment
#version 460

#define TEX_ARRAY_SIZE 512

layout(set = 1, binding = 0) uniform sampler texSampler;
layout(set = 1, binding = 1) uniform texture2D textures[TEX_ARRAY_SIZE];

layout(set = 2, binding = 1) uniform LightSource {
    vec4 position;
    vec4 color;
} light;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) flat in uint texIndex;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(sampler2D(textures[texIndex], texSampler), fragUV);
    
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    float intensity = max(dot(normalize(fragNormal), lightDir), 0.0);
    vec3 diffuse = intensity * light.color.xyz;

    outColor = vec4(texColor.xyz * diffuse, 1.0);
}