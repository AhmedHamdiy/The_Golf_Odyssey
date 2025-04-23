#version 330 core

#include "light_common.glsl"

in Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 world;
    vec3 view;
    vec3 normal;
} fs_in;

uniform TexturedMaterial material;
uniform Light lights[MAX_LIGHT_COUNT];
uniform SkyLightColor sky_light_color;
uniform int light_count;

out vec4 frag_color;

void main() {
    InputVaryings input;
    input.world = fs_in.world;
    input.view = fs_in.view;
    input.normal = fs_in.normal;
    Material sampled_material = sample_material(material, fs_in.tex_coord);
    vec3 accumulated_light = calculate_accumulated_light(sampled_material, lights, sky_light_color, input, light_count);

    frag_color = fs_in.color * vec4(accumulated_light, 1.0f);
}