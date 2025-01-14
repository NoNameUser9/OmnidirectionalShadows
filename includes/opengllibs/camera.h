#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Определяет возможные направления движения камеры. Используется для абстракции ввода,
// чтобы избежать зависимости от конкретной системы ввода окна.
enum Camera_Movement {
    FORWARD,    // Движение вперед
    BACKWARD,   // Движение назад
    LEFT,       // Движение влево
    RIGHT       // Движение вправо
};

// Стандартные значения камеры
const float YAW         = -90.0f;   // Начальный угол обзора по оси Y (в градусах)
const float PITCH       =  0.0f;    // Начальный угол наклона камеры по оси X (в градусах)
const float SPEED       =  2.5f;    // Скорость движения камеры
const float SENSITIVITY =  0.1f;    // Чувствительность мыши
const float ZOOM        =  45.0f;   // Начальный уровень зума камеры

// Абстрактный класс камеры, который обрабатывает ввод и вычисляет углы Эйлера, векторы и матрицы для OpenGL
class Camera
{
public:
    // Атрибуты камеры
    glm::vec3 Position;  // Позиция камеры в 3D-пространстве
    glm::vec3 Front;     // Направление, в котором смотрит камера
    glm::vec3 Up;        // Вектор "вверх", определяющий ориентацию камеры
    glm::vec3 Right;     // Вектор "вправо", получаемый как перекрестие "вперед" и "вверх"
    glm::vec3 WorldUp;   // Вектор "вверх" в мировых координатах (не зависит от ориентации камеры)

    // Углы Эйлера, которые управляют ориентацией камеры
    float Yaw;   // Угол поворота по оси Y (горизонтальная ориентация)
    float Pitch; // Угол наклона камеры по оси X (вертикальная ориентация)

    // Параметры камеры
    float MovementSpeed;    // Скорость перемещения камеры
    float MouseSensitivity; // Чувствительность мыши
    float Zoom;             // Уровень зума камеры

    // Конструктор с векторами
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW, float pitch = PITCH)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
          MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors(); // Инициализация направлений камеры
    }

    // Конструктор с индивидуальными значениями для позиции и углов
    Camera(float posX, float posY, float posZ,
           float upX, float upY, float upZ,
           float yaw, float pitch)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
          MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors(); // Инициализация направлений камеры
    }

    // Возвращает матрицу вида, вычисленную с использованием углов Эйлера и матрицы LookAt
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up); // Возвращает матрицу камеры для OpenGL
    }

    // Обрабатывает ввод с клавиатуры, чтобы двигать камеру в 3D-пространстве
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime; // Вычисление скорости в зависимости от времени
        if (direction == FORWARD)
            Position += Front * velocity;   // Движение вперед
        if (direction == BACKWARD)
            Position -= Front * velocity;  // Движение назад
        if (direction == LEFT)
            Position -= Right * velocity;  // Движение влево
        if (direction == RIGHT)
            Position += Right * velocity;  // Движение вправо
    }

    // Обрабатывает ввод с мыши для изменения ориентации камеры
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;  // Умножение на чувствительность мыши
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;  // Обновление угла поворота по оси Y
        Pitch += yoffset;  // Обновление угла наклона камеры по оси X

        // Ограничение угла наклона камеры, чтобы избежать переворота
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        updateCameraVectors(); // Обновляем направления камеры после изменения углов
    }

    // Обрабатывает прокрутку мыши для изменения уровня зума
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset; // Изменяем уровень зума на основе прокрутки
        if (Zoom < 1.0f)  // Ограничиваем минимальный зум
            Zoom = 1.0f;
        if (Zoom > 45.0f) // Ограничиваем максимальный зум
            Zoom = 45.0f;
    }

private:
    // Вычисляет векторы камеры (Front, Right, Up) на основе углов Эйлера
    void updateCameraVectors()
    {
        // Пересчитываем вектор направления (Front) с использованием углов Эйлера
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));  // x-компонент
        front.y = sin(glm::radians(Pitch));                           // y-компонент
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));  // z-компонент
        Front = glm::normalize(front);  // Нормализуем вектор направления (Front)

        // Пересчитываем вектор "вправо" (Right) как перекрестие "Front" и "WorldUp"
        Right = glm::normalize(glm::cross(Front, WorldUp));
        // Пересчитываем вектор "вверх" (Up) как перекрестие "Right" и "Front"
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

#endif
