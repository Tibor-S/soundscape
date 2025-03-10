#version 450


layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(binding = 1) uniform Modification {
    mat4 model;
} pos;

layout(binding = 2) uniform BoneBuffer{
    mat4 bone[2];
} bon;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in int boneIndex;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 viewDir;

void main() {
    gl_Position = ubo.proj * ubo.view * bon.bone[boneIndex] * pos.model * vec4(inPosition, 1.0);
    fragNormal = inNormal;
    viewDir = vec3(ubo.view[0][2], ubo.view[1][2], ubo.view[2][2]);
}