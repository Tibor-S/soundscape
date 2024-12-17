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
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in int boneIndex;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 viewDir;

void main() {
    vec4 pos = ubo.proj * ubo.view * bon.bone[boneIndex] * pos.model * vec4(inPosition, 1.0);
    gl_Position = pos;
    fragColor = inColor;
    fragNormal = inNormal;
    fragPos = pos.xyz;
    viewDir = vec3(ubo.view[0][2], ubo.view[1][2], ubo.view[2][2]);

}