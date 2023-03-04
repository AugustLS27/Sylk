#version 330 core

out vec4 pixel;

in vec3 colors;
in vec2 tex_coords;

uniform sampler2D tex1;
uniform sampler2D tex2;

uniform float mix_val;

void main() {
    pixel = mix(texture(tex1, tex_coords), texture(tex2, vec2(1.0 - tex_coords.x, tex_coords.y)), mix_val);
}