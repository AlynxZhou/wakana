#version 300 es
precision mediump float;

in vec3 pass_color;

out vec4 out_color;

void main()
{
	out_color = vec4(pass_color, 1.0f);
}
