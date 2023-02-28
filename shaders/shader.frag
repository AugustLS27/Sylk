#version 330 core

out vec4 pixel;
in vec3 out_color;

void main() {
    pixel = vec4(out_color, 1.0);
}