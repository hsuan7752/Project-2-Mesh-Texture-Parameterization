#include <Common.h>
#include <ViewManager.h>
#include <AntTweakBar/AntTweakBar.h>
#include <ResourcePath.h>

#include "OpenMesh.h"
#include "MeshObject.h"
#include "DrawModelShader.h"
#include "PickingShader.h"
#include "PickingTexture.h"
#include "DrawPickingFaceShader.h"
#include "DrawTextureShader.h"
#include "DrawPointShader.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../Include/STB/stb_image.h"

#include <Eigen/Sparse>

using namespace glm;
using namespace std;
using namespace Eigen;

glm::vec3 worldPos;
bool updateFlag = false;
bool isRightButtonPress = false;
GLuint currentFaceID = 0;
int currentMouseX = 0;
int currentMouseY = 0;
int windowWidth = 1200;
int windowHeight = 600;
const std::string ProjectName = "TextureParameterization";

GLuint			program;			// shader program
mat4			proj_matrix;		// projection matrix
float			aspect;
ViewManager		meshWindowCam;

vector < MeshObject > textures;

MeshObject model;
MeshObject SelectedModel;

// shaders
DrawModelShader drawModelShader;
DrawPickingFaceShader drawPickingFaceShader;
PickingShader pickingShader;
PickingTexture pickingTexture;
DrawPointShader drawPointShader;

// vbo for drawing point
GLuint vboPoint;

int mainWindow;
enum SelectionMode
{
	ADD_FACE,
	DEL_FACE,
	SELECT_POINT,
	ADD_RING
};

enum SelectionTexture
{
	CHECKERBOARD4,
	NONE
};

SelectionMode selectionMode = ADD_FACE;
SelectionTexture selectionTex = CHECKERBOARD4;

TwBar* bar;
TwEnumVal SelectionModeEV[] = { {ADD_FACE, "Add face"}, {DEL_FACE, "Delete face"}, {SELECT_POINT, "Point"}, {ADD_RING, "Add ring"}};
TwEnumVal SelectionTexEV[] = { {CHECKERBOARD4, "Checkboard4"}, {NONE, "None"}};
TwType SelectionModeType;
TwType SelectionTexType;

unsigned int textureID;
static float textureU = 0, textureV = 0, textureR = 0;

class TEXTURE
{
	unsigned int texture_id = 0;

};

void SetupGUI()
{
#ifdef _MSC_VER
	TwInit(TW_OPENGL, NULL);
#else
	TwInit(TW_OPENGL_CORE, NULL);
#endif
	TwGLUTModifiersFunc(glutGetModifiers);
	bar = TwNewBar("Texture Parameter Setting");
	TwDefine(" 'Texture Parameter Setting' size='220 90' ");
	TwDefine(" 'Texture Parameter Setting' fontsize='3' color='96 216 224'");

	// Defining season enum type
	SelectionModeType = TwDefineEnum("SelectionModeType", SelectionModeEV, 4);
	// Adding season to bar
	TwAddVarRW(bar, "SelectionMode", SelectionModeType, &selectionMode, NULL);

	SelectionTexType = TwDefineEnum("SelectionTexType", SelectionTexEV, 2);
	// Adding season to bar
	TwAddVarRW(bar, "SelectionTexType", SelectionTexType, &selectionTex, NULL);
}

void My_LoadModel()
{
	if (model.Init(ResourcePath::modelPath))
	{
		/*int id = 0;
		while (model.AddSelectedFace(id))
		{
			++id;
		}
		model.Parameterization();
		drawTexture = true;*/

		puts("Load Model");
	}
	else
	{
		puts("Load Model Failed");
	}
}

unsigned int My_LoadTexture(string path, int imageType)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load image, create texture and generate mipmaps
	int width, height, nrComponents;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, imageType, width, height, 0, imageType, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "texture failed to load at path: " << path << std::endl;
	}

	stbi_image_free(data);
	
	return textureID;
}

void InitOpenGL()
{
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_POINT_SMOOTH);
}

void InitData()
{
	ResourcePath::shaderPath = "./Shader/" + ProjectName + "/";
	//ResourcePath::imagePath = "./Imgs/TextureParameterization/checkerboard4.jpg";
	ResourcePath::modelPath = "./Model/armadillo.obj";

	//Initialize shaders
	///////////////////////////	
	drawModelShader.Init();
	pickingShader.Init();
	pickingTexture.Init(windowWidth, windowHeight);
	drawPickingFaceShader.Init();
	drawPointShader.Init();

	glGenBuffers(1, &vboPoint);

	//Load model to shader program
	My_LoadModel();

	textureID = My_LoadTexture("Imgs\\TextureParameterization\\checkerboard4.jpg", GL_RGB);
}

void Reshape(int width, int height)
{
	windowWidth = width;
	windowHeight = height;

	TwWindowSize(width, height);
	glutReshapeWindow(windowWidth, windowHeight);
	glViewport(0, 0, windowWidth, windowHeight);

	aspect = windowWidth * 1.0f / windowHeight;
	meshWindowCam.SetWindowSize(windowWidth, windowHeight);
	pickingTexture.Init(windowWidth, windowHeight);
}

// GLUT callback. Called to draw the scene.
void RenderMeshWindow()
{
	//Update shaders' input variable
	///////////////////////////	

	glm::mat4 mvMat = meshWindowCam.GetViewMatrix() * meshWindowCam.GetModelMatrix();
	glm::mat4 pMat = meshWindowCam.GetProjectionMatrix(aspect);

	// write faceID+1 to framebuffer
	pickingTexture.Enable();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	pickingShader.Enable();
	pickingShader.SetMVMat(value_ptr(mvMat));
	pickingShader.SetPMat(value_ptr(pMat));

	model.Render();

	pickingShader.Disable();
	pickingTexture.Disable();

	
	// draw model
	glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawModelShader.Enable();
	glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));
	glm::mat4 trans = glm::mat4(1.0f);
	trans = glm::rotate(trans, glm::radians(0.0f), glm::vec3(0.0, 0.0, 1.0));
	//trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));

	drawModelShader.SetWireColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	drawModelShader.SetFaceColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	drawModelShader.UseLighting(true);
	drawModelShader.DrawWireframe(true);
	drawModelShader.SetNormalMat(normalMat);
	drawModelShader.SetMVMat(mvMat);
	drawModelShader.SetPMat(pMat);
	drawModelShader.SetUVRotMat(trans);

	model.Render();

	if (model.GetSelectedVertexSize() > 0)
	{
		drawModelShader.SetWireColor(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
		drawModelShader.SetFaceColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
	
		drawModelShader.DrawTexCoord(true);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
	
		drawModelShader.DrawTexture(true);
		drawModelShader.DrawWireframe(false);
		drawModelShader.SetTexcoord(textureU, textureV, textureR);
		SelectedModel.Render();
		drawModelShader.DrawTexture(false);
		drawModelShader.DrawWireframe(true);
	}

	drawModelShader.Disable();
	
	// render selected face
	if (selectionMode == SelectionMode::ADD_FACE || selectionMode == SelectionMode::DEL_FACE || selectionMode == SelectionMode::ADD_RING)
	{
		drawPickingFaceShader.Enable();
		drawPickingFaceShader.SetMVMat(value_ptr(mvMat));
		drawPickingFaceShader.SetPMat(value_ptr(pMat));
		model.RenderSelectedFace();
		drawPickingFaceShader.Disable();
	}

	glUseProgram(0);

	// render closest point
	if (selectionMode == SelectionMode::SELECT_POINT)
	{
		if (updateFlag)
		{
			float depthValue = 0;
			int windowX = currentMouseX;
			int windowY = windowHeight - currentMouseY;
			glReadPixels(windowX, windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthValue);

			GLint _viewport[4];
			glGetIntegerv(GL_VIEWPORT, _viewport);
			glm::vec4 viewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
			glm::vec3 windowPos(windowX, windowY, depthValue);
			glm::vec3 wp = glm::unProject(windowPos, mvMat, pMat, viewport);
			model.FindClosestPoint(currentFaceID - 1, wp, worldPos);			

			drawPickingFaceShader.Enable();
			drawPickingFaceShader.SetMVMat(value_ptr(mvMat));
			drawPickingFaceShader.SetPMat(value_ptr(pMat));
			if (model.GetSelectedVertexSize() == 0)
				model.RenderSelectedFace();
			drawPickingFaceShader.Disable();

			updateFlag = false;
		}
		/*
			Using OpenGL 1.1 to draw point
		*/
		/*glPushMatrix();
		glPushAttrib(GL_POINT_BIT);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMultMatrixf(glm::value_ptr(pMat));

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glMultMatrixf(glm::value_ptr(mvMat));

			glPointSize(15.0f);
			glColor3f(1.0, 0.0, 1.0);
			glBegin(GL_POINTS);
			glVertex3fv(glm::value_ptr(worldPos));
			glEnd();
		glPopAttrib();
		glPopMatrix();*/

		
		glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), glm::value_ptr(worldPos), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glm::vec4 pointColor(1.0, 0.0, 1.0, 1.0);
		drawPointShader.Enable();
		drawPointShader.SetMVMat(mvMat);
		drawPointShader.SetPMat(pMat);
		drawPointShader.SetPointColor(pointColor);
		drawPointShader.SetPointSize(15.0);

		glDrawArrays(GL_POINTS, 0, 1);

		drawPointShader.Disable();

		glBindBuffer(GL_ARRAY_BUFFER, 0);

	}

	glBindTexture(GL_TEXTURE_2D, textureID);
	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	float tempx = -0.5;
	float tempy = 0.5;
	float Nx = cos(-textureR) * tempx - sin(-textureR) * tempy + 0.5 - textureU;
	float Ny = sin(-textureR) * tempx + cos(-textureR) * tempy + 0.5 + textureV;
	glTexCoord2d(Nx, Ny); glVertex2d(0, -1);
	tempx = 1 - 0.5;
	tempy = 1 - 0.5;
	Nx = cos(-textureR) * tempx - sin(-textureR) * tempy + 0.5 - textureU;
	Ny = sin(-textureR) * tempx + cos(-textureR) * tempy + 0.5 + textureV;
	glTexCoord2d(Nx, Ny); glVertex2d(1, -1);
	tempx = 1 - 0.5;
	tempy = 0 - 0.5;
	Nx = cos(-textureR) * tempx - sin(-textureR) * tempy + 0.5 - textureU;
	Ny = sin(-textureR) * tempx + cos(-textureR) * tempy + 0.5 + textureV;
	glTexCoord2d(Nx, Ny); glVertex2d(1, 1);
	tempx = 0 - 0.5;
	tempy = 0 - 0.5;
	Nx = cos(-textureR) * tempx - sin(-textureR) * tempy + 0.5 - textureU;
	Ny = sin(-textureR) * tempx + cos(-textureR) * tempy + 0.5 + textureV;
	glTexCoord2d(Nx, Ny); glVertex2d(0, 1);
	glEnd();

	
	glBegin(GL_POINTS);
	glColor3f(0, 1, 0);
	glPointSize(100);
	
	for (MyMesh::VertexIter vertex_it = SelectedModel.model.mesh.vertices_begin(); vertex_it != SelectedModel.model.mesh.vertices_end(); ++vertex_it)
	{
		MyMesh::VHandle DrawPoint1 = SelectedModel.model.mesh.vertex_handle(vertex_it->idx());
		MyMesh::TexCoord2D outPoint1_p = SelectedModel.model.mesh.texcoord2D(DrawPoint1);
		glVertex3f(outPoint1_p[0], outPoint1_p[1] * 2 - 1, 0);
	}
	glEnd();

	glBegin(GL_LINES);
	glColor3f(1, 0, 0);
	glLineWidth(10);
	for (MyMesh::EdgeIter edge_it = SelectedModel.model.mesh.edges_begin(); edge_it != SelectedModel.model.mesh.edges_end(); ++edge_it)
	{
		MyMesh::EdgeHandle edge_handle = SelectedModel.model.mesh.edge_handle(edge_it->idx());
		MyMesh::HalfedgeHandle halfedge_handle = SelectedModel.model.mesh.halfedge_handle(edge_handle, 0);
		MyMesh::VHandle DrawPoint1 = SelectedModel.model.mesh.from_vertex_handle(halfedge_handle);
		MyMesh::VHandle DrawPoint2 = SelectedModel.model.mesh.to_vertex_handle(halfedge_handle);
		MyMesh::TexCoord2D outPoint1_p = SelectedModel.model.mesh.texcoord2D(DrawPoint1);
		MyMesh::TexCoord2D outPoint2_p = SelectedModel.model.mesh.texcoord2D(DrawPoint2);
		glVertex3f(outPoint1_p[0], outPoint1_p[1] * 2 - 1, 0);
		glVertex3f(outPoint2_p[0], outPoint2_p[1] * 2 - 1, 0);
	}
	glEnd();

	TwDraw();
	glutSwapBuffers();
}

void RenderAll()
{
	RenderMeshWindow();
}


//Timer event
void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(16, My_Timer, val);
}

void SelectionHandler(unsigned int x, unsigned int y)
{
	GLuint faceID = pickingTexture.ReadTexture(x, windowHeight - y - 1);
	if (faceID != 0)
	{
		currentFaceID = faceID;
	}

	if (selectionMode == ADD_FACE)
	{
		if (faceID != 0)
		{
			model.AddSelectedFace(faceID - 1);
		}
	}
	else if (selectionMode == DEL_FACE)
	{
		if (faceID != 0)
		{
			model.DeleteSelectedFace(faceID - 1);
		}
	}
	else if (selectionMode == SELECT_POINT)
	{
		currentMouseX = x;
		currentMouseY = y;
		updateFlag = true;
	}
	else if (selectionMode == ADD_RING)
	{
		if (faceID != 0)
		{
			model.AddSelectedFace(faceID - 1);

			MyMesh::FaceHandle face_handle = model.model.mesh.face_handle(faceID - 1);
			MyMesh::FFIter face_face_it = model.model.mesh.ff_begin(face_handle);
			for (; face_face_it != model.model.mesh.ff_end(face_handle); ++face_face_it)
			{
				model.AddSelectedFace(face_face_it->idx());
				MyMesh::FaceHandle tmp_face_handle = model.model.mesh.face_handle(face_face_it->idx());
				MyMesh::FFIter tmp_face_face_it = model.model.mesh.ff_begin(tmp_face_handle);
				for (; tmp_face_face_it != model.model.mesh.ff_end(tmp_face_handle); ++tmp_face_face_it)
					model.AddSelectedFace(tmp_face_face_it->idx());
			}
		}
	}
}

//Mouse event
void MyMouse(int button, int state, int x, int y)
{
	if (!TwEventMouseButtonGLUT(button, state, x, y))
	{
		meshWindowCam.mouseEvents(button, state, x, y);

		if (button == GLUT_RIGHT_BUTTON)
		{
			if (state == GLUT_DOWN)
			{
				isRightButtonPress = true;
				SelectionHandler(x, y);
			}
			else if (state == GLUT_UP)
			{
				isRightButtonPress = false;
			}
		}
	}
}

#define PI 3.1415936

double countAngle(MyMesh::Point from, MyMesh::Point to, MyMesh::Point p)
{
	double vec1_x = from[0] - p[0];
	double vec1_y = from[1] - p[1];
	double vec1_z = from[2] - p[2];

	double vec2_x = p[0] - to[0];
	double vec2_y = p[1] - to[1];
	double vec2_z = p[2] - to[2];

	double dot = vec1_x * vec2_x + vec1_y * vec2_y + vec1_z * vec2_z;
	double len_vec1 = sqrt(pow(vec1_x, 2) + pow(vec1_y, 2) + pow(vec1_z, 2));
	double len_vec2 = sqrt(pow(vec2_x, 2) + pow(vec2_y, 2) + pow(vec2_z, 2));

	double angle = acos(dot / (len_vec1 * len_vec2));

	angle = angle * 180.0 / PI;

	return angle;
}

void Texture()
{
	model.FaceToVertex(); // change selected face to vertex

	vector< MyMesh::VertexHandle> vHandle;

	// add selected mesh vertex to new mesh
	for (int i = 0; i < model.GetSelectedVertexSize(); ++i)
	{
		MyMesh::VertexHandle vertex_handle = model.model.mesh.vertex_handle(model.GetSelectedVertexID(i));
		MyMesh::Point p = model.model.mesh.point(vertex_handle);

		vHandle.push_back(SelectedModel.model.mesh.add_vertex(p));
	}

	//cout << vHandle.size() << endl;

	vector<MyMesh::VertexHandle> face_vHandle;
	
	// add selected mesh face to new mesh
	for (int i = 0; i < model.vertexSequence.size() / 3; i++)
	{
		face_vHandle.clear();
		face_vHandle.push_back(vHandle[model.vertexSequence[i * 3]]);
		face_vHandle.push_back(vHandle[model.vertexSequence[i * 3 + 1]]);
		face_vHandle.push_back(vHandle[model.vertexSequence[i * 3 + 2]]);
		SelectedModel.model.mesh.add_face(face_vHandle);
	}
	
	//cout << model.GetSelectedFaceSize() << endl;
	
	//cout << SelectedModel.model.mesh.n_faces() << endl;
	//cout << SelectedModel.model.mesh.n_edges() << endl;

	//model.PrintVertexSequence();

	// find one selected mesh boundary vertex
	vector< unsigned int > boundaryVertex;
	MyMesh::HalfedgeIter halfedge_it = SelectedModel.model.mesh.halfedges_begin();
	MyMesh::HalfedgeHandle halfedge_handle;
	for (; halfedge_it != SelectedModel.model.mesh.halfedges_end(); ++halfedge_it)
	{
		halfedge_handle = SelectedModel.model.mesh.halfedge_handle(halfedge_it->idx());
	
		if (SelectedModel.model.mesh.is_boundary(halfedge_handle))
		{
			MyMesh::VertexHandle vertex_handle = SelectedModel.model.mesh.from_vertex_handle(halfedge_handle);
			boundaryVertex.push_back(vertex_handle.idx());
			break;
		}
	}

	//find other selected mesh boundary vertex
	MyMesh::HalfedgeHandle next_halfedge_handle = SelectedModel.model.mesh.next_halfedge_handle(halfedge_handle);
	while (true)
	{
		MyMesh::VertexHandle vertex_handle = SelectedModel.model.mesh.from_vertex_handle(next_halfedge_handle);

		if (vertex_handle.idx() == boundaryVertex[0])
			break;

		boundaryVertex.push_back(vertex_handle.idx());
		next_halfedge_handle = SelectedModel.model.mesh.next_halfedge_handle(next_halfedge_handle);
	}

	//cout << boundaryVertex.size() << endl;	

	// count boundary total distance
	float totalDistance = 0;
	float oneBoundaryDistance = 0;
	vector< float > dis;	
	for (int i = 0; i < boundaryVertex.size(); ++i)
	{
		MyMesh::VertexHandle vertex_handle1 = SelectedModel.model.mesh.vertex_handle(boundaryVertex[i % boundaryVertex.size()]);
		MyMesh::VertexHandle vertex_handle2 = SelectedModel.model.mesh.vertex_handle(boundaryVertex[(i + 1) % boundaryVertex.size()]);

		MyMesh::Point p1 = SelectedModel.model.mesh.point(vertex_handle1);
		MyMesh::Point p2 = SelectedModel.model.mesh.point(vertex_handle2);

		MyMesh::Point distance_point = p1 - p2;
		float distance = pow(distance_point[0], 2) + pow(distance_point[1], 2) + pow(distance_point[2], 2);
		distance = sqrtf(distance);
		dis.push_back(distance);

		totalDistance += distance;
	}

	oneBoundaryDistance = totalDistance / 4.0;

	SelectedModel.model.mesh.request_vertex_texcoords2D();

	// mapping selected mesh boundary to texture  boundary
	MyMesh::TexCoord2D tempUV(0, 0);
	SelectedModel.model.mesh.set_texcoord2D(SelectedModel.model.mesh.vertex_handle(boundaryVertex[0]), tempUV);
	float nowDistance = 0;	 
	for (int i = 0; i < dis.size(); ++i)
	{
		MyMesh::VertexHandle vertex_handle = SelectedModel.model.mesh.vertex_handle(boundaryVertex[(i + 1) % dis.size()]);
		nowDistance += dis[i];
		if (nowDistance <= oneBoundaryDistance)
		{
			MyMesh::TexCoord2D UV(0, nowDistance / oneBoundaryDistance);
			SelectedModel.model.mesh.set_texcoord2D(vertex_handle, UV);
		}
		else if (nowDistance <= oneBoundaryDistance * 2)
		{
			MyMesh::TexCoord2D UV((nowDistance - oneBoundaryDistance) / oneBoundaryDistance, 1);
			SelectedModel.model.mesh.set_texcoord2D(vertex_handle, UV);
		}
		else if (nowDistance <= oneBoundaryDistance * 3)
		{
			MyMesh::TexCoord2D UV(1, 1 - (nowDistance - oneBoundaryDistance * 2) / oneBoundaryDistance);
			SelectedModel.model.mesh.set_texcoord2D(vertex_handle, UV);
		}
		else
		{
			MyMesh::TexCoord2D UV(1 - (nowDistance - oneBoundaryDistance * 3) / oneBoundaryDistance, 0);
			SelectedModel.model.mesh.set_texcoord2D(vertex_handle, UV);
		}
	}

	// find all selected mesh interior vertex
	vector< unsigned int > interiorVertex;	
	for (MyMesh::VertexIter vertex_it = SelectedModel.model.mesh.vertices_begin(); vertex_it != SelectedModel.model.mesh.vertices_end(); ++vertex_it)
	{
		if (find(boundaryVertex.begin(), boundaryVertex.end(), vertex_it->idx()) == boundaryVertex.end())
			interiorVertex.push_back(vertex_it->idx());
	}

	//cout << interiorVertex.size() << endl;
	
	SparseMatrix <double> A(interiorVertex.size(), interiorVertex.size());
	
	VectorXd Bx(interiorVertex.size());
	VectorXd By(interiorVertex.size());

	// count weight of edges & add to edges
	OpenMesh::EPropHandleT <double> W_i;
	SelectedModel.model.mesh.add_property(W_i);	
	for (MyMesh::EdgeIter edge_it = SelectedModel.model.mesh.edges_begin(); edge_it != SelectedModel.model.mesh.edges_end(); ++edge_it)
	{
		double temp_W_i;
	
		MyMesh::EdgeHandle edge_handle = SelectedModel.model.mesh.edge_handle(edge_it->idx());		
		if (!SelectedModel.model.mesh.is_boundary(edge_handle))
		{
			MyMesh::HalfedgeHandle halfedge_handle = SelectedModel.model.mesh.halfedge_handle(edge_handle, 0);
		
			MyMesh::VertexHandle vertex_handle_from = SelectedModel.model.mesh.from_vertex_handle(halfedge_handle);
			MyMesh::VertexHandle vertex_handle_to = SelectedModel.model.mesh.to_vertex_handle(halfedge_handle);
			MyMesh::VertexHandle vertex_handle_opposite = SelectedModel.model.mesh.opposite_vh(halfedge_handle);
			MyMesh::VertexHandle vertex_handle_opposite_opposite = SelectedModel.model.mesh.opposite_he_opposite_vh(halfedge_handle);
		
			MyMesh::Point vertex_from = SelectedModel.model.mesh.point(vertex_handle_from);
			MyMesh::Point vertex_to = SelectedModel.model.mesh.point(vertex_handle_to);
			MyMesh::Point vertex_opposite = SelectedModel.model.mesh.point(vertex_handle_opposite);
			MyMesh::Point vertex_opposite_opposite = SelectedModel.model.mesh.point(vertex_handle_opposite_opposite);
		
			double angle1 = 0, angle2 = 0;
			
			angle1 = countAngle(vertex_from, vertex_to, vertex_opposite);
			angle2 = countAngle(vertex_from, vertex_to, vertex_opposite_opposite);
		
			temp_W_i = 1 / (angle1 * PI / 180.0) + 1 / (angle2 * PI / 180.0);
		
			SelectedModel.model.mesh.property(W_i, *edge_it) = temp_W_i;
		}
	}

	for (int i = 0; i < interiorVertex.size(); ++i)
	{
		MyMesh::VertexHandle vertex_handle = SelectedModel.model.mesh.vertex_handle(interiorVertex[i]);
	
		double totalWeight = 0.0;
	
		for (MyMesh::VVIter vertex_vertex_it = SelectedModel.model.mesh.vv_begin(vertex_handle); vertex_vertex_it != SelectedModel.model.mesh.vv_end(vertex_handle); ++vertex_vertex_it)
		{			
			MyMesh::VertexHandle target_vertex_handle = SelectedModel.model.mesh.vertex_handle(vertex_vertex_it->idx());
			MyMesh::HalfedgeHandle halfedge_handle = SelectedModel.model.mesh.find_halfedge(vertex_handle, target_vertex_handle);
			MyMesh::EdgeHandle edge_handle = SelectedModel.model.mesh.edge_handle(halfedge_handle);
			double weight = SelectedModel.model.mesh.property(W_i, edge_handle);
			if (SelectedModel.model.mesh.is_boundary(target_vertex_handle))
			{
				MyMesh::TexCoord2D textureCoord = SelectedModel.model.mesh.texcoord2D(target_vertex_handle);
				Bx[i] += weight * textureCoord[0];
				By[i] += weight * textureCoord[1];
			}
			else
			{
				vector< unsigned int >::iterator it = find(interiorVertex.begin(), interiorVertex.end(), vertex_vertex_it->idx());
				int index = std::distance(interiorVertex.begin(), it);
				A.insert(i, index) = -weight;
			}
			totalWeight += weight;
		}
		A.insert(i, i) = totalWeight;
	}
	
	A.makeCompressed();

	if (interiorVertex.size() > 0)
	{
		SparseQR<SparseMatrix<double>, COLAMDOrdering<int>> linearSolver;
		linearSolver.compute(A);
		VectorXd Xx = linearSolver.solve(Bx);
		linearSolver.compute(A);
		VectorXd Xy = linearSolver.solve(By);

		for (int i = 0; i < interiorVertex.size(); ++i)
		{
			MyMesh::VertexHandle vertex_handle = SelectedModel.model.mesh.vertex_handle(interiorVertex[i]);

			MyMesh::TexCoord2D UV(Xx[i], Xy[i]);

			SelectedModel.model.mesh.set_texcoord2D(vertex_handle, UV);

			SelectedModel.model.mesh.request_face_normals();
			SelectedModel.model.mesh.update_normals();
			SelectedModel.model.mesh.release_face_normals();

			SelectedModel.model.TexCoord = true;
			SelectedModel.model.LoadToShader();
		}
	}	

	//cout << SelectedModel.model.mesh.n_vertices() << endl;
}

void delete_Texture()
{
	model.ClearSelectedFace();
	model.ClearSelectedVertex();
	model.ClearVertexSequence();

	SelectedModel.model.mesh.clear();
	SelectedModel.model.mesh.ClearMesh();
}

//Keyboard event
void MyKeyboard(unsigned char key, int x, int y)
{
	if (!TwEventKeyboardGLUT(key, x, y))
	{
		meshWindowCam.keyEvents(key);
	}

	if (key == 'f')
	{
		SelectedModel.model.mesh.clear();
		SelectedModel.model.mesh.ClearMesh();
		Texture();
		//textures.push_back(SelectedModel);
		model.ClearSelectedFace();
		
	}

	if (key == 'g')
	{
		delete_Texture();
	}
}


void MyMouseMoving(int x, int y) {
	if (!TwEventMouseMotionGLUT(x, y))
	{
		meshWindowCam.mouseMoveEvent(x, y);

		if (isRightButtonPress)
		{
			SelectionHandler(x, y);
		}
	}
}

int main(int argc, char* argv[])
{
#ifdef __APPLE__
	//Change working directory to source code path
	chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif

	glutInitWindowPosition(100, 100);
	glutInitWindowSize(windowWidth, windowHeight);
	mainWindow = glutCreateWindow(ProjectName.c_str()); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif

	glutReshapeFunc(Reshape);
	glutIdleFunc(RenderAll);
	glutSetOption(GLUT_RENDERING_CONTEXT, GLUT_USE_CURRENT_CONTEXT);
	SetupGUI();

	glutMouseFunc(MyMouse);
	glutKeyboardFunc(MyKeyboard);
	glutMotionFunc(MyMouseMoving);
	glutDisplayFunc(RenderMeshWindow);
	InitOpenGL();
	InitData();

	//Print debug information 
	Common::DumpInfo();

	// Enter main event loop.
	glutMainLoop();

	return 0;
}