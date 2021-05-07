#include "Shader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

bool Shader::checkShaderCompileError(GLuint shaderPtr)
{
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(shaderPtr, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(shaderPtr, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED" << std::endl << infoLog << std::endl;
	}

	return success;
}

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
	//1. Read shaders
	std::string vertexShaderString;
	std::string fragmentShaderString;
	std::ifstream vertexShaderFile;
	std::ifstream fragmentShaderFile;
	//Allow ifstream objects to throw exceptions
	vertexShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fragmentShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		//Open files
		vertexShaderFile.open(vertexPath, std::ifstream::in);
		fragmentShaderFile.open(fragmentPath, std::ifstream::in);
		//Allows access to read buffers as string
		std::stringstream vertexShaderStream, fragmentShaderStream;
		//Read files into streams and convert to strings
		vertexShaderStream << vertexShaderFile.rdbuf();
		fragmentShaderStream << fragmentShaderFile.rdbuf();
		vertexShaderFile.close();
		fragmentShaderFile.close();
		vertexShaderString = vertexShaderStream.str();
		fragmentShaderString = fragmentShaderStream.str();
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
		std::cout << e.what() << std::endl;
	}
	//Convert to C-String for OpenGL
	const char *vertexShaderSource = vertexShaderString.c_str();
	const char* fragmentShaderSource = fragmentShaderString.c_str();

	//2. Compile shaders
	GLuint vertexShaderPtr, fragmentShaderPtr;
	//Vertex
	vertexShaderPtr = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderPtr, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShaderPtr);
	checkShaderCompileError(vertexShaderPtr);
	//Fragment
	fragmentShaderPtr = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderPtr, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShaderPtr);
	checkShaderCompileError(fragmentShaderPtr);

	//Create shader program
	programId = glCreateProgram();
	glAttachShader(programId, vertexShaderPtr);
	glAttachShader(programId, fragmentShaderPtr);
	glLinkProgram(programId);
	//Check for linking errors
	GLint success;
	char infoLog[512];
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED" << std::endl << infoLog << std::endl;
	}
	//Clean up shaders, they are now contained in the program
	glDeleteShader(vertexShaderPtr);
	glDeleteShader(fragmentShaderPtr);
}

void Shader::use()
{
	glUseProgram(programId);
}

GLuint Shader::getProgramId()
{
	return programId;
}

void Shader::setBool(const std::string& name, bool value) const
{
	glUniform1i(glGetUniformLocation(programId, name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value) const
{
	glUniform1i(glGetUniformLocation(programId, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
	glUniform1f(glGetUniformLocation(programId, name.c_str()), value);
}

void Shader::setVec3fv(const std::string& name, glm::vec3 vector) const
{
	glUniform3fv(glGetUniformLocation(programId, name.c_str()), 1, glm::value_ptr(vector));
}

void Shader::setMat4fv(const std::string& name, glm::mat4 matrix) const
{
	glUniformMatrix4fv(glGetUniformLocation(programId, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}