#include "SaveTexture.h"

JSON::JSON(unsigned int textureID, size_t mesh_n, MyMesh mesh)
{
    j["textureID"] = textureID;
    j["mesh n_faces"] = mesh_n;
    j["mesh vertices id"] = mesh.ids;
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

void DataBase::LoadJSON()
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
        //j = json::parse(inFile);
        cout << j["textureID"] << endl;
        cout << j["mesh n_faces"] << endl;
        vector<unsigned int> t =  j["mesh vertices id"].get<vector<unsigned int>>();
        for (int i = 0; i < t.size(); ++i)
            cout << t[i] << endl;

        TEXTURE textures(j["textureID"], t);
    }
    inFile.close();
}