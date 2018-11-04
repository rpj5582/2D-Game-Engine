#version 430 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 worldPosition;
layout(location = 2) in vec2 scale;
layout(location = 3) in vec4 in_color;

layout(location = 0) uniform mat4 projection;
layout(location = 1) uniform mat4 view;

out vec4 color;

void main()
{
	mat4 world = mat4(
		scale.x,			0,					0,				0,
		0,					scale.y,			0,				0,
		0,					0,					1,				0,
		worldPosition.x,	worldPosition.y,	0,				1
	);

	vec4 finalPosition = projection * view * world * vec4(position, 0.0f, 1.0f);
	gl_Position = finalPosition;

	color = in_color;
}