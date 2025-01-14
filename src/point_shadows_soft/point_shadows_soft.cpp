#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <opengllibs/filesystem.h>
#include <opengllibs/shader.h>
#include <opengllibs/camera.h>
#include <opengllibs/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);
void renderScene(const Shader &shader);
void renderCube();

// settings
const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1600;
bool shadows = true;
bool shadowsKeyPressed = false;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PointShadow", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // настройка глобального состояния OpenGL
    // --------------------------------------
    // GL_DEPTH_TEST - проверка глубины, GL_CULL_FACE - отсечение поверхности
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // сборка и компиляция шейдеров
    // ----------------------------
    Shader shader("point_shadows.vs",
                  "point_shadows.fs");
    Shader simpleDepthShader("point_shadows_depth.vs",
                             "point_shadows_depth.fs",
                             "point_shadows_depth.gs");

    // загрузка текстур
    // ----------------
    unsigned int grassTexture = loadTexture(FileSystem::getPath("resources/textures/grass.jpeg").c_str());

    // настройка карты глубины FBO (Framebuffer Object)
    // ------------------------------------------------
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    // создадим n (n = 1) кадров для карты глубины, где depthMapFBO - переменная, в которой хранится сгенерированное
    // имя объекта буфера кадра (используется массив, если n > 1).
    glGenFramebuffers(1, &depthMapFBO);
    // создание текстуры кубической карты глубины
    // ------------------------------------------
    unsigned int depthCubemap;
    // работает аналогично glGenFramebuffers, но генерирует имена текстур
    glGenTextures(1, &depthCubemap);
    // привязка текстуры кубической карты глубины, где GL_TEXTURE_CUBE_MAP - это тип привязываемой текстуры
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        // заполнение текстуры кубической карты глубины, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i - это индекс текстуры кубической карты глубины
        // GL_DEPTH_COMPONENT - это формат текстуры (указываем что это компонент глубины)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0,
                     GL_DEPTH_COMPONENT,
                     SHADOW_WIDTH,
                     SHADOW_HEIGHT,
                     0,
                     GL_DEPTH_COMPONENT,
                     GL_FLOAT, // GL_FLOAT - тип данных
                     NULL); // NULL - указатель на данные (поскольку мы хотим создать пустую текстуру, мы передаем NULL)
    // установка параметров текстуры кубической карты глубины
    // ------------------------------------------------------
    // GL_TEXTURE_CUBE_MAP - это тип привязываемой текстуры
    // GL_TEXTURE_MIN_FILTER - это алгоритм интерполяции текстуры при уменьшении
    // GL_TEXTURE_MAG_FILTER - это алгоритм интерполяции текстуры при увеличении
    // GL_NEAREST - это алгоритм интерполяции
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R - это алгоритм обработки границ текстуры
    // GL_CLAMP_TO_EDGE - это алгоритм обработки границ текстуры
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Привязываем текстуру глубины в качестве буфера глубины FBO
    // ----------------------------------------------------------
    // GL_FRAMEBUFFER - это тип буфера кадра
    // depthMapFBO - имя объекта буфера кадра, в котором хранится глубина
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    // GL_DEPTH_ATTACHMENT - это тип буфера кадра, в котором хранится глубина
    // depthCubemap - имя текстуры кубической карты глубины
    // 0 - указывает уровень MIP-карты(mipmap) объекта текстуры для присоединения, т.е. уровень 0.
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    // GL_NONE - это индекс текстуры кубической карты глубины (поскольку мы не собираемся использовать эту текстуру
    // для отрисовки, мы передаём GL_NONE)
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    // GL_FRAMEBUFFER - это тип буфера кадра
    // 0 - это индекс буфера кадра
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // настройка шейдеров
    // ------------------
    shader.use();
    shader.setInt("diffuseTexture", 0);
    shader.setInt("depthMap", 1);

    // местоположение источника света
    // ------------------------------
    glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

    // цикл рендеринга
    // ---------------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic (логика обработки времени в расчёте на кадр)
        // -----------------------------------------------------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ввод
        // ----
        processInput(window);

        // перемещать позицию света со временем.
        lightPos.z = static_cast<float>(sin(glfwGetTime() * 0.5) * 3.0);

        // отрисовка
        // ---------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 0. создать матрицы преобразования кубической карты глубины
        // ----------------------------------------------------------
        // shadowProj - это матрица перспективы
        // shadowTransforms - это вектор матриц преобразования
        // каждая матрица преобразования - это матрица перспективы проекции кубической карты глубины на плоскость
        // поверхности, всего их 6 штук, по 1 на каждую сторону проекции кубической карты глубины
        // near_plane - это ближняя плоскость проекции кубической карты глубины
        // far_plane - это дальняя плоскость проекции кубической карты глубины
        float near_plane = 1.0f;
        float far_plane = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

        // 1. рендеринг сцены в кубическую карту глубины
        // ---------------------------------------------
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        simpleDepthShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            simpleDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        simpleDepthShader.setFloat("far_plane", far_plane);
        simpleDepthShader.setVec3("lightPos", lightPos);
        renderScene(simpleDepthShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. отрендерить сцену в обычном режиме
        // -------------------------------------
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        // задать униформы освещения (set lighting uniforms)
        shader.setVec3("lightPos", lightPos);
        shader.setVec3("viewPos", camera.Position);
        shader.setInt("shadows", shadows); // enable/disable shadows by pressing 'SPACE'
        shader.setFloat("far_plane", far_plane);
        // рендерим сцену
        // -------------
        // GL_TEXTURE0 - это индекс текущей текстуры
        // GL_TEXTURE_2D - это тип текстуры (2D текстура)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        renderScene(shader);
        // GL_TEXTURE1 - индекс текущей текстуры
        // GL_TEXTURE_CUBE_MAP - тип текстуры, которая является кубической картой глубины, она похожа на 2D текстуру,
        // но имеет 6 слоев, которые соответствуют направлениям, каждый слой является квадратом.

        // glfw: обменять буферы и обработать события ввода-вывода (нажатия/отпускания клавиш, движение мыши и т. д.).
        // -------------------------------------------------------------------------------
        // glSwapBuffers - это функция из библиотеки GLFW, используемая для управления двойной буферизацией при
        // рендеринге в OpenGL. Она обменивает передний и задний буферы текущего контекста окна, позволяя обновить
        // содержимое окна на экране.
        // glfwPollEvents - это функция из библиотеки GLFW, которая обрабатывает все ожидающие события
        // пользовательского ввода и обновляет внутренние состояния GLFW. Она предназначена для обработки событий без
        // блокировки выполнения программы.
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// renders the 3D scene
// --------------------
void renderScene(const Shader &shader)
{
    // room cube
    // -------
    // присвоим единичную матрицу модели, чтобы рендерить куб в начале координат
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(5.0f));
    shader.setMat4("model", model);
    glDisable(GL_CULL_FACE); // обратите внимание, что мы отключаем отсечение здесь, так как рендерим «внутри» куба,
    // а не снаружи, что сбивает нормальные методы отсечения.
    shader.setInt("reverse_normals", 1); // Небольшой хак для инвертирования нормалей при рендере куба изнутри, чтобы освещение всё равно работало.
    renderCube();
    shader.setInt("reverse_normals", 0); // и, конечно, отключим это
    glEnable(GL_CULL_FACE);
    // cubes
    // -----
    // снова зададим единичную матрицу модели, чтобы сбросить все предыдущие преобразования над матрицей модели (все
    // параметры местоположения и поворота возвращаются к значениям по умолчанию), поскольку все преобразования
    // модели применяются к текущей матрице модели.
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(4.0f, -3.5f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));

    shader.setMat4("model", model);
    renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 3.0f, 1.0));
    model = glm::scale(model, glm::vec3(0.75f));

    shader.setMat4("model", model);
    renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-3.0f, -1.0f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));

    shader.setMat4("model", model);
    renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.5f, 1.0f, 1.5));
    model = glm::scale(model, glm::vec3(0.5f));

    shader.setMat4("model", model);
    renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -3.0));
    model = glm::scale(model, glm::vec3(0.75f));

    shader.setMat4("model", model);
    renderCube();
}

// renderCube() рендерит 1x1 3D-куб в нормализованных координатах устройства (NDC).
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // инициализировать (если необходимо)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // заполним буфер
        // --------------
        // привяжем объект буфера вершин (Vertex Buffer Object) к текущему контексту OpenGL
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        // заполним буфер вершин данными, находящимися в массиве vertices
        // glBufferData — заполняет буфер данными
        // GL_ARRAY_BUFFER — указывает, что мы хотим заполнить буфер вершин
        // sizeof(vertices) — указывает размер нашего массива вершин
        // vertices — указатель на наш массив вершин
        // GL_STATIC_DRAW — указывает, что мы хотим использовать наш буфер вершин только для чтения
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // привяжем атрибуты вершин
        // ------------------------
        // привяжем объект вершинного массива (Vertex Array Object) к текущему контексту OpenGL
        glBindVertexArray(cubeVAO);
        // включаем общий массив атрибутов вершин, указанный индексом (index = 0)
        glEnableVertexAttribArray(0);
        // указываем, какие данные соответствуют каждому атрибуту вершины
        // атрибут 0: координаты вершин
        // 3 значения (x, y, z), тип данных - float (GL_FLOAT), нормализация - false (GL_FALSE), шаг - 8 байт (sizeof(float) *
        // 8), смещение - 0 байт
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        // атрибут 1: нормали вершин
        // 3 значения (x, y, z), тип данных - float (GL_FLOAT), нормализация - false (GL_FALSE), шаг - 8 байт (sizeof(float) *
        // 8), смещение - 3 байта
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        // атрибут 2: текстурные координаты вершин
        // 2 значения (s, t), тип данных - float (GL_FLOAT), нормализация - false (GL_FALSE), шаг - 8 байт (sizeof(float) * 8),
        // смещение - 6 байт
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        // отключаем объект вершинного массива (0 - зарезервированный индекс, означающий обнуление буфера), который
        // подключался ранее (cubeVBO)
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // отключаем объект вершинного массива (cubeVAO)
        glBindVertexArray(0);
    }
    // рендеринг куба
    glBindVertexArray(cubeVAO);
    // функция glDrawArrays рисует массив вершин в режиме треугольников (GL_TRIANGLES), начиная с индекса 0 и с
    // числом вершин 36
    glDrawArrays(GL_TRIANGLES, 0, 36);
    // отключаем объект вершинного массива, после рендеринга
    glBindVertexArray(0);
}

// обработать весь ввод: запросить у GLFW, были ли соответствующие клавиши нажаты/отпущены в этом кадре, и
// соответствующим образом отреагировать.
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    // выход из приложения при нажатии ESC
    // ----------------------------------
    // glfwGetKey — возвращает состояние указанной клавиши
    // GLFW_KEY_ESCAPE — клавиша ESC
    // GLFW_PRESS — клавиша нажата
    // glfwSetWindowShouldClose — устанавливает значение переменной windowShouldClose
    // true при нажатии клавиши ESC
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // процесс движения камеры
    // -----------------------
    // GLFW_KEY_W — клавиша W
    // directional vector (0, 1, 0)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    // directional vector (0, -1, 0)
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    // directional vector (1, 0, 0)
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    // directional vector (-1, 0, 0)
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // устанавливаем включение/выключение теней при нажатии пробела
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !shadowsKeyPressed)
    {
        shadows = !shadows;
        shadowsKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        shadowsKeyPressed = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    // создаем ID текстуры
    // -------------------
    // glGenTextures — генерирует ID текстуры и сохраняет его в переменную textureID, где n — количество текстур,
    // которые мы хотим создать, а textureID — переменная, в которую сохранится ID текстуры
    unsigned int textureID;
    glGenTextures(1, &textureID);

    // параметры текстуры
    // ------------------
    // path - путь к изображению
    // x (width), y (height) — указатели для получения ширины и высоты изображения
    // channels_in_file (nrComponents) — указатель для получения количества каналов изображения (например, 3 для RGB, 4 для RGBA).
    // Может быть NULL, если не нужно получать эту информацию
    // desired_channels (0) — количество каналов в возвращаемом изображении:
    //0: оставить количество каналов как в исходном изображении.
    //1: преобразовать изображение в черно-белое.
    //3: получить RGB.
    //4: получить RGBA.
    int width, height, nrComponents;
    // data - указатель на данные изображения
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        // привязываем текстуру к GL_TEXTURE_2D
        // ------------------------------------
        // glBindTexture — устанавливает текущую текстуру (textureID) для использования в режиме GL_TEXTURE_2D
        glBindTexture(GL_TEXTURE_2D, textureID);
        // загружаем изображение в текстуру
        // -------------------------------
        // glTexImage2D — загружает изображение в текущую текстуру
        // GL_TEXTURE_2D — указываем, что мы хотим использовать текстуру в режиме GL_TEXTURE_2D
        // 0 — указываем уровень мипмап-текстуры, который мы хотим загрузить (0 - оставить уровень по умолчанию)
        // format — указываем формат изображения (например, GL_RGB или GL_RGBA)
        // width, height — указываем ширину и высоту изображения
        // 0 — указываем уровень мипмап-текстуры, который мы хотим загрузить (0 - оставить уровень по умолчанию)
        // format — указываем формат изображения (например, GL_RGB или GL_RGBA)
        // GL_UNSIGNED_BYTE — указываем тип данных изображения
        // data — указываем указатель на данные изображения
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        // генерируем мипмап-текстуру
        glGenerateMipmap(GL_TEXTURE_2D);

        // настройка параметров текстуры
        // ------------------------------
        // glTexParameteri — устанавливает параметры текстуры
        // GL_TEXTURE_WRAP_S/ GL_TEXTURE_WRAP_T — указываем, что мы хотим установить параметры для режима GL_TEXTURE_2D
        // GL_CLAMP_TO_EDGE — указываем, что мы хотим использовать режим GL_CLAMP_TO_EDGE (координаты ограничиваются
        // диапазоном [1/(2N),1−1/(2N)], где N — размер текстуры в направлении ограничения), если format == GL_RGBA
        // иначе указываем, что мы хотим использовать режим GL_REPEAT (повторение изображения)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        // GL_TEXTURE_MIN_FILTER/ GL_TEXTURE_MAG_FILTER — используются для mipmap-текстурирования
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
