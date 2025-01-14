#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <opengllibs/shader.h>

#include <string>
#include <vector>
using namespace std;

// Максимальное количество костей, которые могут влиять на вершину
#define MAX_BONE_INFLUENCE 4

// Структура для представления вершины сетки, включая позиции, нормали, текстурные координаты и информацию о костях для анимации
struct Vertex {
    // Позиция вершины
    glm::vec3 Position;
    // Нормаль вершины
    glm::vec3 Normal;
    // Текстурные координаты
    glm::vec2 TexCoords;
    // Тангенс для нормалей (используется для отображения нормалей и правильной работы с текстурами)
    glm::vec3 Tangent;
    // Битангенс для нормалей
    glm::vec3 Bitangent;
    // Индексы костей, которые будут влиять на эту вершину
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    // Веса от каждой кости, которая влияет на вершину
    float m_Weights[MAX_BONE_INFLUENCE];
};

// Структура для представления текстуры
struct Texture {
    unsigned int id;   // Идентификатор текстуры
    string type;       // Тип текстуры (например, diffuse, specular)
    string path;       // Путь к текстуре на диске
};

// Класс для работы с сеткой, включая рендеринг и обработку данных
class Mesh {
public:
    // Данные сетки: вершины, индексы и текстуры
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<Texture>      textures;
    unsigned int VAO;    // Vertex Array Object (контейнер для всех буферов)

    // Конструктор, инициализирующий сетку
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        // Инициализируем данные для рендеринга
        setupMesh();
    }

    // Метод для отрисовки сетки с использованием шейдера
    void Draw(Shader &shader)
    {
        // Привязываем соответствующие текстуры
        unsigned int diffuseNr  = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr   = 1;
        unsigned int heightNr   = 1;
        for(unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i); // Активируем нужный юнит текстуры перед привязкой
            string number;
            string name = textures[i].type;

            // Определяем номер текстуры для каждого типа (например, texture_diffuse1)
            if(name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "texture_specular")
                number = std::to_string(specularNr++);
            else if(name == "texture_normal")
                number = std::to_string(normalNr++);
            else if(name == "texture_height")
                number = std::to_string(heightNr++);

            // Устанавливаем sampler для соответствующего текстурного юнита
            glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
            // Привязываем текстуру
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        // Отрисовываем сетку
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Сбрасываем все обратно на дефолтные значения
        glActiveTexture(GL_TEXTURE0);
    }

private:
    // Буферы для вершин и индексов
    unsigned int VBO, EBO;

    // Метод для инициализации всех буферов (VBO, EBO) и настройка указателей атрибутов
    void setupMesh()
    {
        // Создаем VAO, VBO и EBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // Привязываем VAO
        glBindVertexArray(VAO);

        // Загружаем данные вершин в VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // Загружаем индексы в EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Устанавливаем указатели на атрибуты вершин
        // Позиции вершин
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // Нормали вершин
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // Текстурные координаты
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // Тангенты
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // Битангенты
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
        // Индексы костей
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

        // Веса костей
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

        // Сбрасываем привязку VAO
        glBindVertexArray(0);
    }
};

#endif
