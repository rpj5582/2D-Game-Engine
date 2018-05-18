#pragma once

#include <GL/glew.h>
#include <FreeImage.h>
#include <unordered_map>

class AssetManager
{
public:
	AssetManager();
	~AssetManager();

	static AssetManager* getInstance();

	unsigned int getTexture(const char* name);
	unsigned int getShader(const char* name);

	unsigned int loadTexture(const char* name, const char* filepath);
	unsigned int loadShader(const char* name, const char* vertexFilepath, const char* fragmentFilepath);

private:
	unsigned int readShader(const char* shaderPath, unsigned int shaderType);
	unsigned int createShaderProgram(unsigned int vertexShader, unsigned int fragmentShader);

	static AssetManager* m_instance;

	std::unordered_map<const char*, unsigned int> m_textureMap;
	std::unordered_map<const char*, unsigned int> m_shaderMap;
	std::unordered_map<const char*, unsigned int> m_shaderProgramMap;
};