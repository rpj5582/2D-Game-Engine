#version 430 core

in vec2 out_uv;

layout(location = 3) uniform sampler2D tex;
layout(location = 5) uniform vec4 tintColor;

out vec4 color;

void main()
{
	color = texture2D(tex, out_uv) * tintColor;
}