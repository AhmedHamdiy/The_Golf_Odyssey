#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;

out Varyings {
    vec3 position;
    vec4 color;
    vec2 tex_coord;
    vec3 normal;
} vs_out;

uniform mat4 M_transform;
uniform mat4 VP_transform;

void main(){
    //TODO: (Req 3) Change the next line to apply the transformation matrix
    // 4x4 matrix * 4x1 vector = 4x1 vector which is the new position
    gl_Position = VP_transform * M_transform * vec4(position,1.0);
    // No need to change any of the following lines
    vs_out.position = position;
    vs_out.color = color;
    vs_out.tex_coord = tex_coord;
    vs_out.normal = normal;
}