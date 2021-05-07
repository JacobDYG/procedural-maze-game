#pragma once
#include <vector>
#include <string>
#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Model
{
private:
	std::vector<Mesh> meshes;
	std::vector<Texture> texturesLoaded;
	std::string directory;

	void loadModel(std::string path);
	void processNode(aiNode *node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName);
public:
	Model(std::string);
	void Draw(Shader& shader);
};

