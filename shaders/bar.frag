#version 450
vec3 sun = normalize(vec3(0, 1.0, 1.0));
//vec3 view = normalize(vec3(0.0, 1.0, 0.0));
float k_a = 0.1;
float k_d = 0.6;
float k_s = 0.3;
float i_a = 1.0;
vec3 i_d = vec3(1.0, 1.0, 1.0);
vec3 i_s = vec3(1.0, 1.0, 1.0);

layout(binding = 3) uniform BarColor {
    vec3 color;
} bc;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 viewDir;

layout(location = 0) out vec4 outColor;

void main() {

    vec3 aux = pow((bc.color - 1), vec3(2));
    vec4 color = vec4(bc.color + 0.05 * aux, 1.0);
    vec3 view = normalize(viewDir);

    vec3 normal = normalize(fragNormal);
    float diff_strength = max(0.0, dot(sun, normal));
    vec3 diff_color = diff_strength * i_d;

    vec3 r = normalize(reflect(-sun, normal));
    float spec_strength = max(0.0, dot(view, r));
    spec_strength = pow(spec_strength, 32.0);
    vec3 spec_color = spec_strength * i_s;
    if (diff_strength <= 0) spec_color = vec3(0.0);

    vec3 light = i_a * k_a + diff_color * k_d + spec_color * k_s;

    outColor = color * vec4(light, 1.0);
//    vec3 r = 2 * dot(sun, normal) * normal - sun;
//    outColor = vec4((k_a * i_a + k_d * max(0, dot(sun, normal)) * i_d) * fragColor, 1.0);
//    outColor = vec4(fragColor * max(base_intensity, dot(sun, normalize(fragNormal))), 1.0);
}