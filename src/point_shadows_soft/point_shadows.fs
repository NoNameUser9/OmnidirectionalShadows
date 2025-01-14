#version 330 core

// Выходной цвет фрагмента
out vec4 FragColor;

// Входные данные от вершинного шейдера (позиция фрагмента, нормаль и текстурные координаты)
in VS_OUT {
    vec3 FragPos;    // Позиция фрагмента в мировых координатах
    vec3 Normal;     // Нормаль фрагмента
    vec2 TexCoords;  // Текстурные координаты фрагмента
} fs_in;

// Униформы: текстуры, позиции света и камеры, флаг теней, дальность отображения
uniform sampler2D diffuseTexture;  // Текстура объекта
uniform samplerCube depthMap;      // Карта теней (кубическая карта)

uniform vec3 lightPos;  // Позиция источника света
uniform vec3 viewPos;   // Позиция камеры

uniform float far_plane; // Дальнейшая граница отображения (используется для преобразования глубины)
uniform bool shadows;    // Флаг, указывающий, нужно ли рассчитывать тени

// Массив направлений смещения для выборки (sampling) теней
vec3 gridSamplingDisk[20] = vec3[](
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1),
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

// Функция для вычисления теней
float ShadowCalculation(vec3 fragPos)
{
    // Получаем вектор от позиции фрагмента (точки на поверхности объекта) до позиции источника света
    vec3 fragToLight = fragPos - lightPos;

    // Получаем текущую линейную глубину фрагмента — это расстояние между фрагментом и источником света
    float currentDepth = length(fragToLight);

    float shadow = 0.0; // Переменная для хранения итогового результата тени
    float bias = 0.15;  // Смещение, используемое для устранения артефактов, таких как "попутные тени"
    int samples = 20;    // Количество сэмплов для фильтрации теней (чем больше, тем мягче тень)
    float viewDistance = length(viewPos - fragPos); // Расстояние от камеры до фрагмента
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0; // Радиус диска для сэмплирования, зависящий от расстояния

    // Процесс сэмплирования по диску, чтобы смягчить тени (PCF)
    for(int i = 0; i < samples; ++i)
    {
        // Сэмплируем глубину с текстуры карты теней на смещенной позиции, умноженной на радиус диска
        float closestDepth = texture(depthMap, fragToLight + gridSamplingDisk[i] * diskRadius).r;

        // Преобразуем глубину в реальные значения, учитывая дальность отображения
        closestDepth *= far_plane;   // Отмена маппинга [0;1] на реальные значения глубины

        // Если текущая глубина фрагмента больше, чем глубина на карте теней (с учетом смещения), то фрагмент в тени
        if(currentDepth - bias > closestDepth)
            shadow += 1.0; // Увеличиваем величину тени
    }

    // Вычисляем среднее значение для мягкой тени (чем больше samples, тем мягче тень)
    shadow /= float(samples);

    // Возвращаем вычисленное значение тени
    return shadow;
}

void main()
{
    // Извлекаем цвет пикселя из текстуры
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;

    // Нормализуем нормаль фрагмента
    vec3 normal = normalize(fs_in.Normal);

    // Определяем цвет света
    vec3 lightColor = vec3(0.3);

    // Амбиентное освещение: базовое освещение, которое всегда присутствует
    vec3 ambient = 0.3 * lightColor;

    // Диффузное освещение: зависит от угла между нормалью и направлением на источник света
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    // Спекулярное освещение: эффект блеска, когда камера и источник света направлены в одну точку
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0); // Используем модель Phong для спекулярного освещения
    vec3 specular = spec * lightColor;

    // Вычисление теней, если они включены
    float shadow = shadows ? ShadowCalculation(fs_in.FragPos) : 0.0;

    // Итоговый цвет, учитывающий амбиентное, диффузное, спекулярное освещение и тени
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

    // Записываем итоговый цвет фрагмента
    FragColor = vec4(lighting, 1.0);
}
