#pragma once

#include "Terrain.h"
#include "Camera.h"

#include <GL/glew.h>

// The amount that should be subtracted from the texture dimensions when rendering to fix gridlike artifacts
#define TEXTURE_SHRINK_FACTOR FLT_EPSILON * 10

struct Vertex
{
	glm::vec2 position;
	glm::vec2 uv;
};

class Renderer
{
public:
	Renderer();
	~Renderer();

	unsigned int getVertexBufferID() const;
	unsigned int getIndexBufferID() const;

	void render(const Camera& camera, const Sprite* sprites, size_t spriteCount);
	void render(const Camera& camera, const Terrain& terrain);

private:
	unsigned int m_vao;
	unsigned int m_vertexBuffer;
	unsigned int m_indexBuffer;
};