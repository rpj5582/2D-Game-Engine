#version 430 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

layout(location = 0) uniform mat4 projection;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 world;

out vec2 out_uv;

void main()
{
	vec4 finalPosition = projection * view * world * vec4(position, 0.0f, 1.0f);
	gl_Position = finalPosition;

	out_uv = uv;
}