#version 450
vec3 sun = normalize(vec3(0.5, 1.0, 2.0));
//vec3 view = normalize(vec3(0.0, 1.0, 0.0));
float k_a = 0.1;
float k_d = 0.9;
float k_s = 0.0;
float i_a = 1.0;
vec3 i_d = vec3(1.0, 1.0, 1.0);
vec3 i_s = vec3(1.0, 1.0, 1.0);

float left_bound = 5;
float right_bound = -5;
float top_bound = 3;
float bot_bound = -1.35482;
float back_bound = -1;
float front_bound = 3;

layout(binding = 1) uniform Colors {
    vec4 color[5];
//    vec3 bot_left;
//    vec3 bot_right;
//    vec3 top_left;
//    vec3 top_right;
//    vec3 front;
} corner_colors;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 viewDir;

layout(location = 0) out vec4 outColor;
//
//vec3 RED = vec3(1.0, 0, 0);
//vec3 GREEN = vec3(0.0, 1.0, 0);
//vec3 BLUE = vec3(0.0, 0, 1.0);
//vec3 WHITE = vec3(1.0, 1.0, 1.0);

void main() {
    float vertical_step = smoothstep(bot_bound, top_bound, fragPos.z);
    float horizontal_step = smoothstep(right_bound, left_bound, fragPos.x);
    float forward_step = smoothstep(back_bound, front_bound, fragPos.y);

    vec4 left_color = mix(corner_colors.color[0], corner_colors.color[2], vertical_step); //mix(corner_colors.bot_left, corner_colors.top_left, vertical_step);
    vec4 right_color = mix(corner_colors.color[1], corner_colors.color[3], vertical_step);
    vec4 back_color = mix(right_color, left_color, horizontal_step);
    vec4 color = mix(back_color, corner_colors.color[4], forward_step);
//    if (fragPos.x < 0) {
//        color = corner_colors.bot_right;
//    } else {
//        color = corner_colors.bot_left;
//    }
//    if (fragPos.z > 1 && fragPos.x < 0) {
//        color = corner_colors.top_right;
//    } else if (fragPos.z > 1 && fragPos.x > 0) {
//        color = corner_colors.top_left;
//    }
//    if (fragPos.z < 0) {
//        color = corner_colors.front;
//    }
//    vec3 left_color = mix(RED, RED, vertical_step); //mix(corner_colors.bot_left, corner_colors.top_left, vertical_step);
//    vec3 right_color = mix(BLUE, BLUE, vertical_step);
//    vec3 back_color = mix(right_color, left_color, horizontal_step);
//    vec3 color = mix(back_color, WHITE, forward_step);

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
//    outColor = vec4(fragNormal, 1.0);
    //    vec3 r = 2 * dot(sun, normal) * normal - sun;
    //    outColor = vec4((k_a * i_a + k_d * max(0, dot(sun, normal)) * i_d) * fragColor, 1.0);
    //    outColor = vec4(fragColor * max(base_intensity, dot(sun, normalize(fragNormal))), 1.0);
}