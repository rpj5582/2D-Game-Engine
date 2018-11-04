#include "stdafx.h"
#include "RenderSystem.h"

#include "TransformSystem.h"

#include "../Terrain.h"

#include <GL/glew.h>

static const Vertex quadVertices[4] =
{
	glm::vec2(-0.5f, -0.5f), glm::vec2(0.0f, 0.0f),
	glm::vec2(0.5f, -0.5f), glm::vec2(1.0f, 0.0f),
	glm::vec2(-0.5f, 0.5f), glm::vec2(0.0f, 1.0f),
	glm::vec2(0.5f, 0.5f), glm::vec2(1.0f, 1.0f)
};

static const unsigned char quadIndices[6] =
{
	0, 1, 2,
	2, 1, 3
};

RendererSystem::RendererSystem()
{
	// Create the VAO to use for most sprites
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	// Vertex buffer
	glGenBuffers(1, &m_vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);

	// Index buffer
	glGenBuffers(1, &m_indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);

	// Populate the vertex and index buffers. These are static, since all quads are the same.
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4, quadVertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned char) * 6, quadIndices, GL_STATIC_DRAW);

	// Setup vertex attribute pointers
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 2));

	// We don't need these since we're only working with 2D
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// Enable blending for transparent sprites
	/*glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/

	// Cornflower blue
	glClearColor(0.392157f, 0.584314f, 0.929412f, 1);

	AssetManager::getInstance()->loadShader("defaultShader", "shaders/vertexShader.glsl", "shaders/fragmentShader.glsl");
}

RendererSystem::~RendererSystem()
{
	glDeleteBuffers(1, &m_vertexBuffer);
	glDeleteBuffers(1, &m_indexBuffer);
	glDeleteVertexArrays(1, &m_vao);
}

void RendererSystem::initComponent(Renderable& renderable, const Texture& texture, glm::vec2 tileDimensions, unsigned int shaderID, unsigned int uvOffsetIndex, glm::vec2 uvOffsets[MAX_ANIMATION_LENGTH])
{
	renderable.texture = texture;
	renderable.tileDimensions = tileDimensions;
	renderable.shaderID = shaderID;
	renderable.uvOffsetIndex = uvOffsetIndex;
	
	if (uvOffsets)
		memcpy_s(renderable.uvOffsets, sizeof(glm::vec2) * MAX_ANIMATION_LENGTH, uvOffsets, sizeof(glm::vec2) * MAX_ANIMATION_LENGTH);
	else
		memset(renderable.uvOffsets, 0, sizeof(glm::vec2) * MAX_ANIMATION_LENGTH);
}

void RendererSystem::destroyComponent(Renderable& renderable)
{
}

unsigned int RendererSystem::getVertexBufferID() const
{
	return m_vertexBuffer;
}

unsigned int RendererSystem::getIndexBufferID() const
{
	return m_indexBuffer;
}

void RendererSystem::render(const Camera& camera)
{
	const glm::mat4& projection = camera.getProjectionMatrix();
	const glm::mat4& view = camera.getViewMatrix();

	glBindVertexArray(m_vao);

	for (size_t i = 0; i < m_components.size(); i++)
	{
		// Cache transform
		const Transform* transform = TransformSystem::getInstance()->getComponent(m_components[i].entityID);
		if (transform)
		{
			glm::vec2 position = transform->position;
			glm::vec2 size = transform->size;

			// Cache renderable
			const Renderable& renderable = m_components[i];
			unsigned int shaderID = renderable.shaderID;
			const Texture& texture = renderable.texture;
			glm::vec2 tileDimensions = renderable.tileDimensions;
			const glm::vec2* uvOffsets = renderable.uvOffsets;
			unsigned int uvOffsetIndex = renderable.uvOffsetIndex;

			// Don't render sprites with no shader
			if (shaderID == 0) continue;

			// Use the shader
			glUseProgram(shaderID);

			// Upload a tint color
			glUniform4f(5, 1.0f, 1.0f, 1.0f, 1.0f);

			// Build the sprite's world matrix
			glm::mat4 world = glm::mat4();
			world = glm::translate(world, glm::vec3(position, 0.0f));
			world = glm::scale(world, glm::vec3(size, 0.0f));

			// Upload uniforms
			glUniformMatrix4fv(0, 1, GL_FALSE, &projection[0][0]);
			glUniformMatrix4fv(1, 1, GL_FALSE, &view[0][0]);
			glUniformMatrix4fv(2, 1, GL_FALSE, &world[0][0]);

			glm::vec2 uvOffsetScaleFactor = texture.dimensions / (tileDimensions - glm::vec2(TEXTURE_SHRINK_FACTOR));
			glUniform2fv(4, 1, &uvOffsetScaleFactor[0]);
			glUniform2fv(6, 1, &uvOffsets[uvOffsetIndex][0]);

			// Bind the texture
			glBindTexture(GL_TEXTURE_2D, texture.id);

			// Draw the quad
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, (void*)0);
		}
	}
}
