#version 300 es

precision mediump float;

uniform sampler2D wallpaper;

in vec2 pass_coordinate;
out vec4 out_color;

void main() {
    out_color = texture(wallpaper, pass_coordinate);
}