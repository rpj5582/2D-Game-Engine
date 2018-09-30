#include "stdafx.h"
#include "AssetManager.h"

#include <fstream>

AssetManager* AssetManager::m_instance = nullptr;

AssetManager::AssetManager()
{
	if (!m_instance)
		m_instance = this;
	else
	{
		Output::error("Attempted to create a second AssetManager instance - this is not supported. Use AssetManager::getInstance() instead.");
		exit(EXIT_FAILURE);
	}
}

AssetManager::~AssetManager()
{
	for (auto it = m_textureMap.begin(); it != m_textureMap.end(); it++)
	{
		glDeleteTextures(1, &it->second);
	}
	m_textureMap.clear();

	for (auto it = m_shaderMap.begin(); it != m_shaderMap.end(); it++)
	{
		glDeleteShader(it->second);
	}
	m_shaderMap.clear();

	for (auto it = m_shaderProgramMap.begin(); it != m_shaderProgramMap.end(); it++)
	{
		glDeleteProgram(it->second);
	}
	m_shaderProgramMap.clear();

	m_instance = nullptr;
}

AssetManager* AssetManager::getInstance()
{
	return m_instance;
}

unsigned int AssetManager::getTexture(const char* name)
{
	assert(m_textureMap.find(name) != m_textureMap.end());
	return m_textureMap[name];
}

unsigned int AssetManager::getShader(const char* name)
{
	assert(m_shaderProgramMap.find(name) != m_shaderProgramMap.end());
	return m_shaderProgramMap[name];
}

unsigned int AssetManager::loadTexture(const char* name, const char* filepath)
{
	assert(m_textureMap.find(name) == m_textureMap.end());

	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	FIBITMAP* dib = nullptr;
	unsigned char* textureData;
	unsigned int width, height;

	fif = FreeImage_GetFileType(filepath, 0);
	if (fif == FIF_UNKNOWN)
	{
		fif = FreeImage_GetFIFFromFilename(filepath);

		if (fif == FIF_UNKNOWN)
			return 0;
	}

	//check that the plugin has reading capabilities and load the file
	if (FreeImage_FIFSupportsReading(fif))
		dib = FreeImage_Load(fif, filepath);

	if (!dib)
		return 0;

	textureData = FreeImage_GetBits(dib);
	width = FreeImage_GetWidth(dib);
	height = FreeImage_GetHeight(dib);

	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(dib);
	if(colorType == FIC_RGBALPHA)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, textureData);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, textureData);

	FreeImage_Unload(dib);

	m_textureMap[name] = textureID;
	return textureID;
}

unsigned int AssetManager::loadShader(const char* name, const char* vertexFilepath, const char* fragmentFilepath)
{
	assert(m_shaderProgramMap.find(name) == m_shaderProgramMap.end());

	unsigned int vertexShader = readShader(vertexFilepath, GL_VERTEX_SHADER);
	unsigned int fragmentShader = readShader(fragmentFilepath, GL_FRAGMENT_SHADER);

	if (!vertexShader || !fragmentShader) return 0;

	m_shaderMap[vertexFilepath] = vertexShader;
	m_shaderMap[fragmentFilepath] = fragmentShader;

	unsigned int shaderProgram = createShaderProgram(vertexShader, fragmentShader);

	if (!shaderProgram) return 0;

	m_shaderProgramMap[name] = shaderProgram;
	return shaderProgram;
}

unsigned int AssetManager::readShader(const char* shaderPath, unsigned int shaderType)
{
	if (m_shaderMap.find(shaderPath) != m_shaderMap.end())
		return m_shaderMap[shaderPath];

	std::ifstream ifs(shaderPath, std::ios::in | std::ios::binary | std::ios::ate);
	if (ifs.is_open())
	{
		int filesize = (int)ifs.tellg();
		ifs.seekg(0, ifs.beg);

		char* shaderBuffer = new char[filesize + 1];
		ifs.read(shaderBuffer, filesize);
		shaderBuffer[filesize] = '\0';

		ifs.close();

		unsigned int shader = glCreateShader(shaderType);
		glShaderSource(shader, 1, &shaderBuffer, nullptr);
		glCompileShader(shader);

		delete[] shaderBuffer;

		int success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

		if (!success)
		{
			int errorLogLength;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &errorLogLength);

			char* errorBuffer = new char[errorLogLength];
			glGetShaderInfoLog(shader, errorLogLength, &errorLogLength, errorBuffer);

			glDeleteShader(shader);
			if (shaderType == GL_VERTEX_SHADER)
				Output::error("Failed to compile vertex shader:\n" + std::string(errorBuffer));
			else if (shaderType == GL_FRAGMENT_SHADER)
				Output::error("Failed to compile fragment shader:\n" + std::string(errorBuffer));

			delete[] errorBuffer;
			return 0;
		}

		m_shaderMap[shaderPath] = shader;
		return shader;
	}
	else
	{
		Output::error("Failed to open file " + std::string(shaderPath));
		return 0;
	}
}

unsigned int AssetManager::createShaderProgram(unsigned int vertexShader, unsigned int fragmentShader)
{
	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	int success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

	if (!success)
	{
		int errorLogLength;
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &errorLogLength);

		char* errorBuffer = new char[errorLogLength];
		glGetProgramInfoLog(shaderProgram, errorLogLength, &errorLogLength, errorBuffer);

		glDeleteProgram(shaderProgram);
		Output::error("Failed to link shader program: " + std::string(errorBuffer));

		delete[] errorBuffer;
		return 0;
	}

	return shaderProgram;
}
