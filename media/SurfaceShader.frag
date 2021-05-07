#version 450 core

struct Material
{
	sampler2D diffuseMap;
	sampler2D specularMap;
	float shininess;
};

struct DirectionalLight
{
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight
{
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

#define MAX_POINT_LIGHTS 8

in vec3 fragPosition;
in vec3 normal;
in vec2 textureCoords;

out vec4 FragColor;

uniform vec3 viewPosition;
uniform Material material;
uniform DirectionalLight directionalLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numPointLights = 0;

vec3 calcDirectionalLight(DirectionalLight light, vec3 normalUnitV, vec3 viewDirection)
{
	vec3 lightDirection = normalize(-light.direction);
	//Ambient
	vec3 ambient = vec3(texture(material.diffuseMap, textureCoords)) * light.ambient;
	//Diffuse
	float diffuseAmount = max(dot(normalUnitV, lightDirection), 0.0);
	vec3 diffuse = diffuseAmount * vec3(texture(material.diffuseMap, textureCoords)) * light.diffuse;
	//Specular
	vec3 reflectDirection = reflect(-lightDirection, normalUnitV);
	float specularAmount = pow(max(dot(viewDirection, reflectDirection), 0.0), material.shininess);
	vec3 specular = vec3(texture(material.specularMap, textureCoords)) * specularAmount * light.specular;

	return (ambient + diffuse + specular);
}

vec3 calcPointLight(PointLight light, vec3 normalUnitV, vec3 fragPosition, vec3 viewDirection)
{
	vec3 lightDirection = normalize(light.position - fragPosition);
	//Ambient
	vec3 ambient = vec3(texture(material.diffuseMap, textureCoords)) * light.ambient;
	//Diffuse
	float diffuseAmount = max(dot(normalUnitV, lightDirection), 0.0);
	vec3 diffuse = diffuseAmount * vec3(texture(material.diffuseMap, textureCoords)) * light.diffuse;
	//Specular
	vec3 reflectDirection = reflect(-lightDirection, normalUnitV);
	float specularAmount = pow(max(dot(viewDirection, reflectDirection), 0.0), material.shininess);
	vec3 specular = vec3(texture(material.specularMap, textureCoords)) * specularAmount * light.specular;
	//Attenuation
	float distance = length(light.position - fragPosition);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;
	return (ambient + diffuse + specular);
}

void main()
{
	vec3 normalUnitV = normalize(normal);
	vec3 viewDirection = normalize(viewPosition - fragPosition);
	
	vec3 result;
	result += calcDirectionalLight(directionalLight, normalUnitV, viewDirection);
	//Consider all point lights
	for(int i = 0; i < numPointLights; i++)
	{
		result += calcPointLight(pointLights[i], normalUnitV, fragPosition, viewDirection);
	}

	FragColor = vec4(result, 1.0);
};