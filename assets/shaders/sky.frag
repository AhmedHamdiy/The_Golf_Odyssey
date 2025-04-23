#version 330 core

#include "light_common.glsl"

in Varyings {
    vec3 view;
} fs_in;

uniform vec4 tint;
uniform sampler2D tex;
uniform SkyLightColor sky_light_color;
uniform float exposure;

out vec4 frag_color;

void main() {
    vec3 view = normalize(fs_in.view);
    vec3 sky_color = exposure * (view.y > 0 ?
        mix(sky_light_color.middle, sky_light_color.top, view.y) :
        mix(sky_light_color.middle, sky_light_color.bottom, -view.y));
    frag_color = vec4(sky_color, 1.0f);
}