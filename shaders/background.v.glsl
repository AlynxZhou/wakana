#version 300 es

in vec2 in_position;
in vec2 in_coordinate;

uniform mat4 mvp;

out vec2 pass_coordinate;

void main() {
    gl_Position = mvp * vec4(in_position, 0.0f, 1.0f);
    pass_coordinate = in_coordinate;
}