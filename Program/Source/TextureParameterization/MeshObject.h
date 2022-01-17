#pragma once

#include <string>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <Common.h>

typedef OpenMesh::TriMesh_ArrayKernelT<>  TriMesh;

class MyMesh : public TriMesh
{
public:
	MyMesh();
	~MyMesh();

	int FindVertex(MyMesh::Point pointToFind);
	void ClearMesh();
};

class GLMesh
{
public:
	GLMesh();
	~GLMesh();

	bool Init(std::string fileName);
	void Render();

	MyMesh mesh;
	GLuint vao;
	GLuint ebo;
	GLuint vboVertices, vboNormal, vboTexcoord;

//private:

	bool LoadModel(std::string fileName);
	void LoadToShader();
	bool TexCoord = false;
};

class MeshObject
{
public:
	MeshObject();
	~MeshObject();

	bool Init(std::string fileName);
	void Render();
	void RenderSelectedFace();
	bool AddSelectedFace(unsigned int faceID);
	void DeleteSelectedFace(unsigned int faceID);
	bool AddSelectedVertex(unsigned int vertexID);
	bool FindClosestPoint(unsigned int faceID, glm::vec3 worldPos, glm::vec3& closestPos);

	unsigned int GetSelectedFaceSize();
	unsigned int GetSelectedVertexSize();

	//Debug
	void PrintSelectedFaceID();
	void PrintSelectedVertexID();
	void PrintVertexSequence();

	unsigned int GetSelectedFaceID(unsigned int index);
	unsigned int GetSelectedVertexID(unsigned int index);

	GLMesh GetModel();
	void FaceToVertex();

	void ClearSelectedFace();
	void ClearSelectedVertex();
	void ClearVertexSequence();

	std::vector<unsigned int> vertexSequence;

	GLMesh model;	
private:	
	std::vector<unsigned int> selectedFace;
	std::vector<unsigned int> selectedVertex;
	std::vector<unsigned int*> fvIDsPtr;
	std::vector<int> elemCount;

};

