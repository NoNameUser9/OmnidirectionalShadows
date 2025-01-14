#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <cstdlib>
#include "root_directory.h" // This is a configuration file generated by CMake.

// Класс для работы с файловой системой, который предоставляет методы для получения путей.
class FileSystem
{
private:
    // Определяет тип функции, которая строит путь. Эта функция принимает строку (путь) и возвращает строку.
    typedef std::string (*Builder)(const std::string& path);

public:
    // Метод для получения пути, используя определенную логику построения пути.
    // В зависимости от корня (например, переменной окружения), выбирается соответствующий метод.
    static std::string getPath(const std::string& path)
    {
        // Статическая переменная, которая хранит указатель на функцию для построения пути.
        static std::string(*pathBuilder)(std::string const &) = getPathBuilder();
        // Вызываем выбранную функцию для построения пути с переданным аргументом.
        return (*pathBuilder)(path);
    }

private:
    // Метод для получения корня файловой системы. Корень может быть взят из переменной окружения
    // или из некоторой константы, определенной в коде.
    static std::string const & getRoot()
    {
        // Переменная окружения, в которой хранится путь к корню.
        static char const * envRoot = getenv("LOGL_ROOT_PATH");
        // Если переменная окружения не пуста, используем её, иначе — используем значение по умолчанию (logl_root).
        static char const * givenRoot = (envRoot != nullptr ? envRoot : logl_root);
        // Если путь существует, сохраняем его в строку. Если нет — пустую строку.
        static std::string root = (givenRoot != nullptr ? givenRoot : "");
        return root; // Возвращаем путь к корню.
    }

    // Метод для получения подходящей функции построения пути в зависимости от наличия корня.
    static Builder getPathBuilder()
    {
        // Если корень задан (не пустой), возвращаем функцию для построения пути относительно корня.
        if (getRoot() != "")
            return &FileSystem::getPathRelativeRoot;
        else
            // Если корень не задан, возвращаем функцию для построения пути относительно бинарного файла.
            return &FileSystem::getPathRelativeBinary;
    }

    // Функция для построения пути относительно заданного корня.
    static std::string getPathRelativeRoot(const std::string& path)
    {
        // Возвращаем путь, объединяя корень с переданным путем.
        return getRoot() + std::string("/") + path;
    }

    // Функция для построения пути относительно бинарного файла.
    static std::string getPathRelativeBinary(const std::string& path)
    {
        // Возвращаем путь, добавляя относительный путь к бинарному файлу.
        return "../../../" + path;
    }
};

// FILESYSTEM_H
#endif
