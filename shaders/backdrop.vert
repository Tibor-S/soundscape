#version 450

vec2 corners[4] = {
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
};

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 viewDir;


void main() {
    vec4 pos = ubo.proj * ubo.view * vec4(inPosition, 1.0);
    gl_Position = pos;
    fragNormal = inNormal;
    fragPos = inPosition;
    viewDir = vec3(ubo.view[0][2], ubo.view[1][2], ubo.view[2][2]);

}