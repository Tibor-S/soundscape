#version 450
vec3 sun = normalize(vec3(1.0, 1.0, 1.0));
float k_a = 0.05;
float k_d = 0.95;
float k_s = 0;
float i_a = 1;
vec3 i_d = vec3(1.0, 1.0, 1.0);
vec3 i_s = vec3(1.0, 1.0, 1.0);

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragView;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 r = 2 * dot(sun, normal) * normal - sun;
    outColor = vec4((k_a * i_a + k_d * max(0, dot(sun, normal)) * i_d + k_s * pow(max(0.0, dot(r, fragView)), 32) * i_s) * fragColor, 1.0);
//    outColor = vec4(fragColor * max(base_intensity, dot(sun, normalize(fragNormal))), 1.0);
}