#pragma once

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <Common.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "json.hpp"
#include "MeshObject.h"

using json = nlohmann::json;
using namespace std;

class JSON
{
public:
	JSON() {}
	JSON(unsigned int textureID, size_t mesh_n, MyMesh mesh);
	json getJson();
	
private:
	json j;
};

class DataBase
{
public:
	void AddData(unsigned int textureID, size_t mesh_n, MyMesh mesh);
	void Clear();
	void SaveJSON();
	void LoadJSON(MeshObject model);
	void buildMesh(vector<unsigned int> _vertices_id, vector<unsigned int> sequence, vector< MyMesh::TexCoord2D > texcoord, MeshObject model, MyMesh &mesh);
	int GetDataSetSize();
	unsigned int GetDataTextureID(int index);

//private:
	vector < JSON > datas;
};