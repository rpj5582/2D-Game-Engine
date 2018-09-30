#version 430 core

#define BLOCK_SIZE 16
#define MAX_ANIMATION_LENGTH 30

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec2 worldPosition;
layout(location = 3) in uint uvOffsetIndex;

layout(location = 0) uniform mat4 projection;
layout(location = 1) uniform mat4 view;
layout(location = 4) uniform vec2 uvOffsetScaleFactor;
layout(location = 6) uniform vec2 uvOffsets[MAX_ANIMATION_LENGTH];

out vec2 out_uv;

void main()
{
	mat4 world = mat4(
		BLOCK_SIZE,			0,					0,				0,
		0,					BLOCK_SIZE,			0,				0,
		0,					0,					BLOCK_SIZE,		0,
		worldPosition.x,	worldPosition.y,	0,				1
	);

	vec4 finalPosition = projection * view * world * vec4(position, 0.0f, 1.0f);
	gl_Position = finalPosition;

	out_uv = (uv + uvOffsets[uvOffsetIndex]) / uvOffsetScaleFactor;
}