#version 450
vec3 sun = normalize(vec3(0, 1.0, 1.0));
//vec3 view = normalize(vec3(0.0, 1.0, 0.0));
float k_a = 0.1;
float k_d = 0.8;
float k_s = 0.1;
float i_a = 1.0;
vec3 i_d = vec3(1.0, 1.0, 1.0);
vec3 i_s = vec3(1.0, 1.0, 1.0);

float left_bound = 5;
float right_bound = -5;
float top_bound = 3;
float bot_bound = -1.35482;
float back_bound = -1;
float front_bound = 3;



float map(float value, float min1, float max1, float min2, float max2) {
    return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float aspect_ratio(vec3 corner1, vec3 corner2) {
    float dx = distance(corner1.x, corner2.x);
    float dy3D = distance(corner1.y, corner2.y);
    float dz3D = distance(corner1.z, corner2.z);

    float dy = dy3D + dz3D;

    return dx / dy;
}

vec2 map_UV(vec3 pos, vec3 origin, vec3 corner1, vec3 corner2) {
    vec3 rpos = pos - origin;
    vec3 rcorner1 = corner1 - origin;
    vec3 rcorner2 = corner2 - origin;

//    float ar = aspect_ratio(rcorner1, rcorner2);

    float xuv = map(rpos.x, rcorner1.x, rcorner2.x, 0.0, 1.0);
    float y = map(rpos.y, rcorner2.y, rcorner1.y, 0.0, 0.5);
    float z = map(rpos.z, rcorner2.z, rcorner1.z, 0.0, 0.5);
    float yuv = y + z;

    return vec2(xuv, yuv);
}

layout(binding = 1) uniform Colors {
    vec4 color[4];
    vec2 fac;
//    vec3 bot_left;
//    vec3 bot_right;
//    vec3 top_left;
//    vec3 top_right;
//    vec3 front;
} palette;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 viewDir;

layout(location = 0) out vec4 outColor;

vec4 RED = vec4(1,0,0,1);
vec4 GRN = vec4(0,1,0,1);
vec4 BLU = vec4(0,0,1,1);

void main() {

//    vec2 uv = map_UV(fragPos, vec3(0.0), vec3(5.0,-1.0,3.0), vec3(-5.0,3.0,-1.35482));
//    float fac = fbm(uv * 5);
//
//    float low = 0;
//    vec4 low_color = palette.color[0];
//    float high = 1;
//    vec4 high_color = palette.color[3];
//    if (fac <= palette.fac.x) {
//        high = palette.fac.x;//palette.fac[0];
//        high_color = palette.color[1];
//    } else if (fac <= palette.fac.y) {
//        low = palette.fac.x;//palette.fac[0];
//        low_color = palette.color[1];
//        high = palette.fac.y;//palette.fac[1];
//        high_color = palette.color[2];
//    } else {
//        low = palette.fac.y;//palette.fac[1];
//        low_color = palette.color[2];
//    }
//    float extended_fac = smoothstep(low, high, fac);
//    vec4 color = mix(low_color, high_color, extended_fac);
    vec4 color = palette.color[0];

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