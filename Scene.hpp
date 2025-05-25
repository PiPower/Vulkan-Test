#pragma once
#include <string>
struct Scene
{
	//const aiScene* scene;
	size_t totalVertexCount;
	size_t totalIndexCount;
};

Scene loadScene(const std::string& path);