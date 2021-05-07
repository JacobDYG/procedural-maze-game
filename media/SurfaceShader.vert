#version 450 core
layout(location = 0) in vec3 aPosition;		//positions
layout(location = 1) in vec3 aNormal;		//normal
layout(location = 2) in vec2 aTextureCoords; //texture coordinates

out vec3 fragPosition;
out vec3 normal;
out vec2 textureCoords;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
	fragPosition = vec3(modelMatrix * vec4(aPosition, 1.0));
	normal = mat3(transpose(inverse(modelMatrix))) * aNormal;

	gl_Position =  projectionMatrix * viewMatrix * vec4(fragPosition, 1.0);
	textureCoords = aTextureCoords;
};