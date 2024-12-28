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

layout(binding = 1) uniform Colors {
    vec3 corners[4]; // bl, br, tl, tr
} corner_colors;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 viewDir;


void main() {
    vec4 pos = ubo.proj * ubo.view * vec4(inPosition, 1.0);
    gl_Position = pos;
    fragNormal = inNormal;
    fragPos = pos.xyz;
    viewDir = vec3(ubo.view[0][2], ubo.view[1][2], ubo.view[2][2]);

    // color
    vec3 color = vec3(0.0);
    float total = 0;
    for (int i = 0; i < 4; i++) {
        float dist = max(1 - length(inTexCoord - corners[i]), 0);
        total += dist;
        color += dist * corner_colors.corners[i];
    }
    fragColor = color / total;
    fragColor = vec3(1.0,0.0,1.0);
}