#pragma once

#define CAMERA_VERTICAL_CLAMP 17408

class Camera
{
public:
	Camera(int width, int height);
	~Camera();

	glm::vec2 getPosition() const;
	int getWidth() const;
	int getHeight() const;

	void translate(glm::vec2 delta);
	void resize(int width, int height);

	const glm::mat4& getViewMatrix() const;
	const glm::mat4& getProjectionMatrix() const;

private:
	glm::mat4 m_projection;
	glm::mat4 m_view;

	glm::vec2 m_position;

	int m_width;
	int m_height;
};