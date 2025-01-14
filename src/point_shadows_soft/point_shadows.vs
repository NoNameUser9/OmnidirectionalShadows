#version 330 core

// Входные данные для вершинного шейдера: координаты вершины, нормаль и текстурные координаты
layout (location = 0) in vec3 aPos;       // Позиция вершины
layout (location = 1) in vec3 aNormal;    // Нормаль вершины
layout (location = 2) in vec2 aTexCoords; // Текстурные координаты вершины

// Выходные данные для фрагментного шейдера
out vec2 TexCoords; // Передаем текстурные координаты

// Структура для передачи нескольких данных во фрагментный шейдер
out VS_OUT {
    vec3 FragPos;  // Позиция фрагмента в мировых координатах
    vec3 Normal;   // Нормаль фрагмента
    vec2 TexCoords; // Текстурные координаты фрагмента
} vs_out;

// Матрицы проекции, вида и модели для преобразования вершин
uniform mat4 projection;  // Матрица проекции
uniform mat4 view;        // Матрица вида (камеры)
uniform mat4 model;       // Матрица модели (позиция и ориентация объекта)

// Флаг для инвертирования нормалей
uniform bool reverse_normals;

void main()
{
    // Преобразуем позицию вершины в мировые координаты и передаем её во фрагментный шейдер
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));

    // Преобразуем нормаль в мировые координаты, с учетом возможного инвертирования
    if(reverse_normals) // Хак для отображения освещения "изнутри" большого куба
        vs_out.Normal = transpose(inverse(mat3(model))) * (-1.0 * aNormal); // Инвертируем нормаль
    else
        vs_out.Normal = transpose(inverse(mat3(model))) * aNormal; // Преобразуем нормаль без инверсии

    // Передаем текстурные координаты
    vs_out.TexCoords = aTexCoords;

    // Преобразуем позицию вершины в экранные координаты
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
