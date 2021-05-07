#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader
{
private:
	GLuint programId;

	bool checkShaderCompileError(GLuint shaderPtr);

public:
	Shader(const char* vertexPath, const char* fragmentPath);
	//Activate the shader
	void use();
	
	//Provide program id read access
	GLuint getProgramId();
	//For setting uniforms within a shader
	void setBool(const std::string &name, bool value) const;
	void setInt(const std::string &name, int value) const;
	void setFloat(const std::string& name, float value) const;
	void setVec3fv(const std::string& name, glm::vec3 vector) const;
	void setMat4fv(const std::string& name, glm::mat4 matrix) const;
};

#endif