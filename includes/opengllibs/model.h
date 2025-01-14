#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <opengllibs/mesh.h>
#include <opengllibs/shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

// Функция для загрузки текстуры из файла
unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model
{
public:
    // Данные модели
    vector<Texture> textures_loaded;  // хранят все загруженные текстуры, чтобы избежать повторной загрузки
    vector<Mesh>    meshes;          // вектор всех мешей модели
    string directory;                 // директория, содержащая модель
    bool gammaCorrection;             // флаг коррекции гамма-цвета

    // Конструктор, который принимает путь к 3D модели
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);  // загрузить модель при создании объекта
    }

    // Метод для рисования модели (всех её мешей)
    void Draw(Shader &shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);  // рисуем каждый меш
    }

private:
    // Метод для загрузки модели с помощью ASSIMP и сохранения полученных мешей в векторе meshes
    void loadModel(string const &path)
    {
        // Чтение файла с помощью ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        // Проверка на ошибки
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // если ошибка
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }

        // Получение директории файла
        directory = path.substr(0, path.find_last_of('/'));

        // Рекурсивная обработка корневого узла ASSIMP
        processNode(scene->mRootNode, scene);
    }

    // Рекурсивный метод обработки узлов. Обрабатывает каждый меш на текущем узле и повторяет для дочерних узлов
    void processNode(aiNode *node, const aiScene *scene)
    {
        // Обработка всех мешей на текущем узле
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // Индекс меша в сцене
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));  // обрабатываем меш и добавляем его в модель
        }

        // Рекурсивно обрабатываем дочерние узлы
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    // Метод для обработки меша: извлекает данные из ASSIMP и создаёт объект Mesh
    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // Данные для заполнения
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        // Обработка каждого вертекса меша
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // временный вектор для передачи данных из ASSIMP в glm::vec3
            // Позиции
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            // Нормали
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }

            // Текстурные координаты
            if(mesh->mTextureCoords[0]) // проверяем наличие текстурных координат
            {
                glm::vec2 vec;
                // Мы предполагаем, что используем только первый набор текстурных координат
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;

                // Тангенты и битангенты
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;

                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f); // если нет текстурных координат, задаём (0,0)

            vertices.push_back(vertex);  // добавляем вертекс в массив
        }

        // Обработка лиц (граней) меша (каждое лицо — это треугольник)
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // Получаем индексы для каждого лица и добавляем их в массив индексов
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Обработка материалов
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // Загрузка текстур для различных типов (диффузные, спекулярные, нормальные, высоты)
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        // Возвращаем объект Mesh, созданный из извлечённых данных
        return Mesh(vertices, indices, textures);
    }

    // Метод для загрузки текстур из материала
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            // Проверка, была ли уже загружена текстура с таким путём
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;  // если текстура уже загружена, пропускаем её загрузку
                    break;
                }
            }
            if(!skip)
            {
                // Если текстура не была загружена, загружаем её
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // сохраняем её как загруженную
            }
        }
        return textures;
    }
};


// Функция для загрузки текстуры из файла
unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
    // Создание полного пути к файлу текстуры
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;  // Переменная для хранения ID текстуры
    glGenTextures(1, &textureID);  // Генерация текстуры

    int width, height, nrComponents;
    // Загрузка изображения с помощью stb_image
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        // Определяем формат текстуры в зависимости от количества компонентов в изображении
        if (nrComponents == 1)
            format = GL_RED;  // Черно-белое изображение
        else if (nrComponents == 3)
            format = GL_RGB;  // Цветное изображение (RGB)
        else if (nrComponents == 4)
            format = GL_RGBA;  // Цветное изображение с альфа-каналом

        // Привязываем текстуру для работы с ней
        glBindTexture(GL_TEXTURE_2D, textureID);
        // Загружаем изображение в текстуру
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        // Генерация мип-маппинга для текстуры
        glGenerateMipmap(GL_TEXTURE_2D);

        // Настройка параметров текстуры
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // Повторение текстуры по оси S
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  // Повторение текстуры по оси T
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);  // Линейная фильтрация с мип-маппингом
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Линейная фильтрация при увеличении

        // Освобождаем память, занятую изображением
        stbi_image_free(data);
    }
    else
    {
        // Если изображение не удалось загрузить, выводим ошибку
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);  // Освобождаем память, если не удалось загрузить
    }

    // Возвращаем ID загруженной текстуры
    return textureID;
}
#endif
