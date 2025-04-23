#version 330 core

layout(location = 0) in vec3 position;

out Varyings {
    vec3 view;
} vs_out;

uniform mat4 VP_transform;
uniform vec3 camera_position;

void main() {
    vs_out.view = position;
    vec4 clip_space = VP_transform * vec4(position + camera_position, 1.0);
    gl_Position = clip_space.xyww;
}
