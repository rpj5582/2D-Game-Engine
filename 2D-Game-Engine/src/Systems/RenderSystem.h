#pragma once
#include "System.h"

// The amount that should be subtracted from the texture dimensions when rendering to fix gridlike artifacts
#define TEXTURE_SHRINK_FACTOR FLT_EPSILON * 10

#include "../Components/Renderable.h"
#include "../Camera.h"

struct Vertex
{
	glm::vec2 position;
	glm::vec2 uv;
};

class Terrain;

class RendererSystem : public System<RendererSystem, Renderable>
{
public:
	RendererSystem();
	~RendererSystem();

	void initComponent(Renderable& renderable, const Texture& texture, glm::vec2 tileDimensions, unsigned int shaderID, unsigned int uvOffsetIndex, glm::vec2 uvOffsets[MAX_ANIMATION_LENGTH]);
	void destroyComponent(Renderable& renderable);

	unsigned int getVertexBufferID() const;
	unsigned int getIndexBufferID() const;

	void render(const Camera& camera);

private:
	unsigned int m_vao;
	unsigned int m_vertexBuffer;
	unsigned int m_indexBuffer;
};
