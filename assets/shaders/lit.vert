#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;

uniform mat4 M_transform;
uniform mat4 M_invT_transform;
uniform mat4 VP_transform;
uniform vec3 camera_position;

out Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 world;
    vec3 view;
    vec3 normal;
} vs_out;

void main(){
    vs_out.world = (M_transform * vec4(position, 1.0f)).xyz;
    vs_out.view = camera_position - vs_out.world;
    vs_out.normal = normalize((M_invT_transform * vec4(normal, 0.0f)).xyz);
    vs_out.color = color;
    vs_out.tex_coord = tex_coord;

    gl_Position = VP_transform * vec4(vs_out.world, 1.0);
}