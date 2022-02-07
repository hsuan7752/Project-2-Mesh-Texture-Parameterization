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
	std::vector<unsigned int> ids;
	std::vector<float> texcoordX;
	std::vector<float> texcoordY;
	std::vector<unsigned int> vertexSequence;
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

	GLuint vaotest, vbo1, vbo2, vbotex, ebotest;

	void SaveToVector(unsigned int textureID, MyMesh mesh);

//private:

	bool LoadModel(std::string fileName);
	void LoadToShader();
	void LoadToShader(GLuint &_vao, GLuint &_ebo, GLuint &_vboVertices, GLuint &_vboNormal, GLuint &_vboTexcoord);
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
	bool DeleteSelectedVertex(unsigned int vertexID);
	bool FindClosestPoint(unsigned int faceID, glm::vec3 worldPos, glm::vec3& closestPos);

	unsigned int GetSelectedFaceSize();
	unsigned int GetSelectedVertexSize();

	//Debug
	void PrintSelectedFaceID();
	void PrintSelectedVertexID();
	void PrintVertexSequence();

	unsigned int GetSelectedFaceID(unsigned int index);
	unsigned int GetSelectedVertexID(unsigned int index);
	void FaceToVertex();

	void ClearSelectedFace();
	void ClearSelectedVertex();
	void ClearVertexSequence();

	

	GLMesh model;

private:	
	std::vector<unsigned int> selectedFace;
	std::vector<unsigned int> selectedVertex;
	std::vector<unsigned int*> fvIDsPtr;
	std::vector<int> elemCount;

};

class TEXTURE
{
public:
	TEXTURE() {}
	TEXTURE(unsigned int _texture_id, MeshObject _mesh);
	TEXTURE(unsigned int _texture_id, MeshObject _mesh, GLuint _vao, GLuint _ebo, GLuint _vboVertices, GLuint _vboNormal, GLuint _vboTexcoord);
	void add(unsigned int _texture_id, MeshObject _mesh);
	void add(unsigned int _texture_id, MyMesh _mesh);
	unsigned int texture_id = 0;


	MeshObject mesh;
	GLuint vao;
	GLuint ebo;
	GLuint vboVertices, vboNormal, vboTexcoord;
};

