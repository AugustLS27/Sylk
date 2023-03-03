#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 tex_coord;

out vec3 colors;
out vec2 tex_coords;

void main() {
    gl_Position = vec4(pos, 1.0);

    colors = color;
    tex_coords = tex_coord;
}