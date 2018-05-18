#include "stdafx.h"
#include "Camera.h"

Camera::Camera(int width, int height)
{
	m_position = glm::vec2();
	glm::vec3 positionVec3 = glm::vec3(m_position, 0.0f);
	m_view = glm::lookAt(positionVec3, positionVec3 + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	resize(width, height);
}

Camera::~Camera()
{
}

void Camera::translate(glm::vec2 delta)
{
	m_position += delta;
	glm::vec3 positionVec3 = glm::vec3(m_position, 0.0f);
	m_view = glm::lookAt(positionVec3, positionVec3 + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::resize(int width, int height)
{
	m_width = width;
	m_height = height;

	m_projection = glm::ortho(-m_width * 0.5f, m_width * 0.5f, -m_height * 0.5f, m_height * 0.5f, 0.0f, 1.0f);
}

const glm::mat4& Camera::getViewMatrix() const
{
	return m_view;
}

const glm::mat4& Camera::getProjectionMatrix() const
{
	return m_projection;
}
