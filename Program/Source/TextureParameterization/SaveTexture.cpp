#include "SaveTexture.h"

JSON::JSON(unsigned int textureID, size_t mesh_n, MyMesh mesh)
{
    j["textureID"] = textureID;
    j["n_faces"] = mesh_n;
    j["id"] = mesh.ids;
    j["sequence"] = mesh.vertexSequence;
    j["texcoordX"] = mesh.texcoordX;
    j["texcoordY"] = mesh.texcoordY;
}

json JSON::getJson()
{
    return j;
}

void DataBase::AddData(unsigned int textureID, size_t mesh_n, MyMesh mesh)
{
    JSON j(textureID, mesh_n, mesh);
    datas.emplace_back(j);
}

void DataBase::Clear()
{
    datas.clear();
}

void DataBase::SaveJSON()
{
    ofstream outFile;
    outFile.open("Mesh.json", ios::out | ios::trunc);

    if (!outFile)
    {
        cout << "File could not be opened!" << endl;
        exit(0);
    }

    for (int i = 0; i < datas.size(); ++i)
    {
        outFile << datas[i].getJson();
        if (i != datas.size() - 1)
            outFile << endl;
    }

    outFile.close();
}

void DataBase::LoadJSON(MeshObject model)
{
    ifstream inFile;
    inFile.open("Mesh.json", ios::in);

    if (!inFile)
    {
        cout << "File could not be opened!" << endl;
        exit(0);
    }

    json j;

    while (inFile.peek() != EOF)
    {
        inFile >> j;
        //cout << j["textureID"] << endl;
        //cout << j["mesh n_faces"] << endl;
        //cout << j["texcoord"] << endl;
        vector<unsigned int> id =  j["id"].get<vector<unsigned int>>();
        vector<unsigned int> sequence = j["sequence"].get <vector<unsigned int>>();
        vector< float > texcoordX = j["texcoordX"].get<vector< float>>();
        vector< float > texcoordY = j["texcoordY"].get<vector< float>>();

        vector< MyMesh::TexCoord2D> texcoord;

        for (int i = 0; i < texcoordX.size(); ++i)
        {
            MyMesh::TexCoord2D tex;
            tex[0] = texcoordX[i];
            tex[1] = texcoordY[i];
            texcoord.push_back(tex);
        }       

        MyMesh mesh;
        buildMesh(id, sequence, texcoord, model, mesh);
        mesh.ids = id;
        mesh.vertexSequence = sequence;
        mesh.texcoordX = texcoordX;
        mesh.texcoordY = texcoordY;
        JSON J(j["textureID"], mesh.n_faces(), mesh);
        datas.emplace_back(J);
    }
    inFile.close();
}

void DataBase::buildMesh(vector<unsigned int> vertices_id, vector<unsigned int> sequence, vector< MyMesh::TexCoord2D > texcoord, MeshObject model, MyMesh &mesh)
{
    mesh.request_vertex_texcoords2D();

    vector< MyMesh::VertexHandle> vHandle;

    // add selected mesh vertex to new mesh
    for (int i = 0; i < vertices_id.size(); ++i)
    {
        MyMesh::VertexHandle vertex_handle = model.model.mesh.vertex_handle(vertices_id[i]);
        MyMesh::Point p = model.model.mesh.point(vertex_handle);

        //mesh.set_texcoord2D(p, texcoord[i]);

        vHandle.push_back(mesh.add_vertex(p));
        mesh.set_texcoord2D(vHandle[vHandle.size() -1], texcoord[i]);
    }

    vector<MyMesh::VertexHandle> face_vHandle;

    // add selected mesh face to new mesh
    for (int i = 0; i < sequence.size() / 3; i++)
    {
        face_vHandle.clear();
        face_vHandle.push_back(vHandle[sequence[i * 3]]);
        face_vHandle.push_back(vHandle[sequence[i * 3 + 1]]);
        face_vHandle.push_back(vHandle[sequence[i * 3 + 2]]);
        mesh.add_face(face_vHandle);
    }


}

int DataBase::GetDataSetSize()
{
    return datas.size();
}

unsigned int DataBase::GetDataTextureID(int index)
{
    return datas.at(index).getJson();
}