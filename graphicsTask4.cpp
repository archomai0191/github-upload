#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <vector>
#include <algorithm>

#define EPSILON 1.0e-5 //граница для 0
#define IS_ZERO(v) (abs(v) < EPSILON) //проверка на равенство 0
#define SIGN(v) (int)(((v) > EPSILON) - ((v) < -EPSILON)) // функция sign(x)
#define RESOLUTION 32
#define C 2.0 // параметр кривизны (должен быть >= 2)
#define pi 3.14152653589793
#define znam 1200

using namespace std;
using namespace glm;

const GLchar vshR[] =
"#version 330\n"
""
"layout(location = 0) in vec3 a_position;"
""
"uniform mat4 u_mv;"
"uniform mat4 u_mvp;"

"out vec3 v_pos;"
"out vec3 v_normal;"
""
"void main()"
"{"
"    gl_Position = u_mvp * vec4(a_position, 1.0);"
"    v_pos = vec3(u_mv * vec4(a_position, 1.0));"
//нормали
"    vec3 n = vec3(-a_position[1], a_position[0], a_position[2]);"
"    n = n * (a_position [0] >= 0 ? 1.0 : -1.0);"
"    v_normal = transpose(inverse(mat3(u_mv))) * normalize(n);"
"}"
;

const GLchar fshR[] =
"#version 330\n"
""
"in vec3 v_pos;"
"in vec3 v_normal;"
""
"layout(location = 0) out vec4 o_color;"
""
"void main()"
"{"
"   vec3 L = vec3(-5.0, 4.0, 4.0);"
"   vec3 E = vec3(0.0, 0.0, 0.0);"
"   vec3 n = normalize(v_normal);"
"   vec3 l = normalize(L - v_pos);"
"   vec3 e = normalize(E - v_pos);"

"   float d = max(1.0, dot(l, n));"
"   vec3 h = normalize(l + e);"
"   float s = max(0.0, dot(n, h));"
"   s = pow(s, 20);"
"   vec4 color = vec4((1.0, 0.0, 0.0) * d + vec3(s), 1.0);"
"   o_color.rgb = pow(color.rgb, vec3(1.0 / 2.2));"
//"   o_color.rgb = vec3(1.0, 0.0, 0.0);"
"   o_color.a = 1.0;"
"}";

GLint w = 800, h = 600;

GLFWwindow* g_window;

GLuint g_shaderProgramRotate, g_shaderProgramPoint, g_shaderProgramLine;

class Model
{
public:
    GLuint vbo;
    GLuint ibo;
    GLuint vao;
    GLsizei indexCount;
} g_modelRotate, g_modelPoint, g_modelLine;

class point
{
public:
    double x, y;
    point(double x, double y)
    {
        this->x = x;
        this->y = y;
    }
    point() { x = y = 0.0; }
    bool operator < (const point& p) const { return x < p.x; }; // сравнение точек
    point operator +(const point& p) const { return point(x + p.x, y + p.y); }; // сложение текущей и переданной точки
    point operator -(const point& p) const { return point(x - p.x, y - p.y); }; // вычитание переданной точки из текущей
    point operator *(double v) const { return point(x * v, y * v); }; // умножение точки на число

    void normalize() //нормализация координат точки
    {
        double l = sqrt(x * x + y * y);
        if (IS_ZERO(l)) x = y = 0.0;
        else
        {
            x /= l;
            y /= l;
        }
    }

    static point absMin(const point& p1, const point& p2) //поиск абсолютного минимума (значения с минимальным модулем)
    {
        return point(abs(p1.x) < abs(p2.x) ? p1.x : p2.x, abs(p1.y) < abs(p2.y) ? p1.y : p2.y);
    }
};

class Segment
{
public:
    point points[4]; // контрольные точки

    point calc(double t) const //расчет промежуточных точек (t - параметр кривой)
    {
        double t2 = t * t;
        double t3 = t2 * t;
        double nt = 1.0 - t;
        double nt2 = nt * nt;
        double nt3 = nt2 * nt;
        return point(nt3 * points[0].x + 3.0 * t * nt2 * points[1].x + 3.0 * t2 * nt * points[2].x + t3 * points[3].x,
            nt3 * points[0].y + 3.0 * t * nt2 * points[1].y + 3.0 * t2 * nt * points[2].y + t3 * points[3].y);
    }
};

vector<point> points, bezierPoints;

GLuint createShader(const GLchar* code, GLenum type)
{
    GLuint result = glCreateShader(type);

    glShaderSource(result, 1, &code, NULL);
    glCompileShader(result);

    GLint compiled;
    glGetShaderiv(result, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 0)
        {
            char* infoLog = (char*)alloca(infoLen);
            glGetShaderInfoLog(result, infoLen, NULL, infoLog);
            cout << "Shader compilation error" << endl << infoLog << endl;
        }
        glDeleteShader(result);
        return 0;
    }

    return result;
}

GLuint createProgram(GLuint vsh, GLuint fsh)
{
    GLuint result = glCreateProgram();

    glAttachShader(result, vsh);
    glAttachShader(result, fsh);

    glLinkProgram(result);

    GLint linked;
    glGetProgramiv(result, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(result, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 0)
        {
            char* infoLog = (char*)alloca(infoLen);
            glGetProgramInfoLog(result, infoLen, NULL, infoLog);
            cout << "Shader program linking error" << endl << infoLog << endl;
        }
        glDeleteProgram(result);
        return 0;
    }

    return result;
}
GLint g_MV, g_MVP;

bool createShaderProgram()
{
    g_shaderProgramPoint = 0;
    g_shaderProgramLine = 0;

    const GLchar vshP[] =
        "#version 330\n"
        ""
        "layout(location = 0) in vec2 a_position;"
        "uniform mat4 u_MV;"
        ""
        "void main()"
        "{"
        "    gl_Position = u_MV * vec4(a_position, 0.0, 1.0);"
        "}"
        ;

    const GLchar fshP[] =
        "#version 330\n"
        ""
        "layout(location = 0) out vec4 o_color;"
        ""
        "void main()"
        "{"
        "   o_color = vec4(1.0, 0.0, 0.0, 1.0);"
        "}"
        ;

    const GLchar vshL[] =
        "#version 330\n"
        ""
        "layout(location = 0) in vec2 a_position;"
        ""
        "void main()"
        "{"
        "    gl_Position = vec4(a_position, 0.0, 1.0);"
        "}"
        ;

    const GLchar fshL[] =
        "#version 330\n"
        ""
        "layout(location = 0) out vec4 o_color;"
        ""
        "void main()"
        "{"
        "   o_color = vec4(0.0, 0.5, 0.8, 1.0);"
        "}"
        ;

    GLuint vertexShader, fragmentShader;

    vertexShader = createShader(vshP, GL_VERTEX_SHADER);
    fragmentShader = createShader(fshP, GL_FRAGMENT_SHADER);

    g_shaderProgramPoint = createProgram(vertexShader, fragmentShader);
    g_MV = glGetUniformLocation(g_shaderProgramPoint, "u_MV");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    vertexShader = createShader(vshL, GL_VERTEX_SHADER);
    fragmentShader = createShader(fshL, GL_FRAGMENT_SHADER);

    g_shaderProgramLine = createProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return g_shaderProgramPoint != 0 && g_shaderProgramLine != 0;
}

bool createModelLine()
{
    GLfloat* pointsToDraw = new GLfloat[bezierPoints.size() * 2];
    GLuint* indices = new GLuint[bezierPoints.size()];
    int i = 0;
    for (point p : bezierPoints) {
        pointsToDraw[i] = p.x;
        pointsToDraw[i + 1] = p.y;
        indices[i / 2] = i / 2;
        i = i + 2;
    }
    glGenVertexArrays(1, &g_modelLine.vao);
    glBindVertexArray(g_modelLine.vao);

    glGenBuffers(1, &g_modelLine.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_modelLine.vbo);
    glBufferData(GL_ARRAY_BUFFER, 2 * bezierPoints.size() * sizeof(GLfloat), pointsToDraw, GL_STATIC_DRAW);

    glGenBuffers(1, &g_modelLine.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_modelLine.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, bezierPoints.size() * sizeof(GLuint), indices, GL_STATIC_DRAW);
    g_modelLine.indexCount = bezierPoints.size();

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const GLvoid*)0);

    delete[] pointsToDraw;
    delete[] indices;

    return g_modelLine.vbo != 0 && g_modelLine.ibo != 0 && g_modelLine.vao != 0;
}

void formVerticesArray(GLfloat* vertices, int rows, int columns)
{
    for (int i = 0; i < rows; i++)
    {
        glm::mat4 rotMatr = glm::rotate(float(i * pi / znam), glm::vec3(0.0, 1.0, 0.0));
        for (int j = 0; j < columns; j++)
        {
            glm::vec4 rotVec = glm::vec4(bezierPoints[j].x, bezierPoints[j].y, 1.0, 0.0);
            rotVec = rotMatr * rotVec;
            vertices[3 * columns * i + 3 * j] = rotVec[0];
            vertices[3 * columns * i + 3 * j + 1] = rotVec[1];
            vertices[3 * columns * i + 3 * j + 2] = rotVec[2];
        }
    }
}

void formIndicesArray(GLuint* indices, int columns, int lines)
{
    for (int i = 0; i < lines; i++)
        for (int j = 0; j < columns; j++)
        {
            indices[6 * columns * i + 6 * j] = indices[6 * columns * i + 6 * j + 5] = (columns + 1) * i + j;
            indices[6 * columns * i + 6 * j + 1] = (columns + 1) * i + j + 1;
            indices[6 * columns * i + 6 * j + 2] = indices[6 * columns * i + 6 * j + 3] = i != lines ? (columns + 1) * (i + 1) + j + 1 : indices[j + 1];
            indices[6 * columns * i + 6 * j + 4] = i != lines ? (columns + 1) * (i + 1) + j : indices[j];
        }
}

bool createModelRotate()
{
    GLfloat* vertices = new GLfloat[bezierPoints.size() * (2 * znam + 1) * 3];
    GLuint* indices = new GLuint[2 * znam * (bezierPoints.size() - 1) * 2 * 3];

    formVerticesArray(vertices, 2 * znam + 1, bezierPoints.size());
    formIndicesArray(indices, bezierPoints.size() - 1, 2 * znam);

    glGenVertexArrays(1, &g_modelRotate.vao);
    glBindVertexArray(g_modelRotate.vao);

    glGenBuffers(1, &g_modelRotate.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_modelRotate.vbo);
    glBufferData(GL_ARRAY_BUFFER, bezierPoints.size() * (2 * znam + 1) * 3 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &g_modelRotate.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_modelRotate.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2 * znam * (bezierPoints.size() - 1) * 2 * 3 * sizeof(GLuint), indices, GL_STATIC_DRAW);
    g_modelRotate.indexCount = 2 * znam * (bezierPoints.size() - 1) * 2 * 3;

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const GLvoid*)0);

    delete[] vertices;
    delete[] indices;
    return g_modelRotate.vbo != 0 && g_modelRotate.ibo != 0 && g_modelRotate.vao != 0;
}

bool createModelPoint() {
    const GLfloat vertices[] = {
          -0.5f, -0.5,
          0.5f, -0.5f,
          0.5f, 0.5f,
          -0.5f, 0.5f
    };

    const GLuint indices[] = {
            0, 1, 2, 2, 3, 0
    };

    glGenVertexArrays(1, &g_modelPoint.vao);
    glBindVertexArray(g_modelPoint.vao);

    glGenBuffers(1, &g_modelPoint.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_modelPoint.vbo);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &g_modelPoint.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_modelPoint.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), indices, GL_STATIC_DRAW);
    g_modelPoint.indexCount = 6;

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const GLvoid*)0);

    return g_modelPoint.vbo != 0 && g_modelPoint.ibo != 0 && g_modelPoint.vao != 0;
}

bool init()
{
    // Set initial color of color buffer to white.
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    return createShaderProgram() && createModelPoint();
}

void reshape(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void draw(bool mode, int chisl)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(g_shaderProgramPoint);
    glBindVertexArray(g_modelPoint.vao);

    if (!mode) //отрисовка кривой
    {
        for (auto& point : points) {
                mat4 view = translate(mat4(1.0), vec3(point.x, point.y, 0.0f));
                view = scale(view, vec3(0.01f));
                glUniformMatrix4fv(g_MV, 1, GL_FALSE, &view[0][0]);
                glDrawElements(GL_TRIANGLES, g_modelPoint.indexCount, GL_UNSIGNED_INT, NULL);
            }

        glUseProgram(g_shaderProgramLine);
        glBindVertexArray(g_modelLine.vao);
        glDrawElements(GL_LINE_STRIP, g_modelLine.indexCount, GL_UNSIGNED_INT, NULL);
    }
    else //отрисовка тела вращения
    {
        glUseProgram(g_shaderProgramRotate);
        glBindVertexArray(g_modelRotate.vao);

        glm::mat4 scale = glm::scale(glm::vec3(0.5, 0.5, 0.5));
        glm::mat4 translate = glm::translate(glm::vec3(0.0, 0.0, 0.0));
        glm::mat4 rotateX = glm::rotate(float(- pi / 6.0), glm::vec3(1.0, 0.0, 0.0));
        glm::mat4 rotateY = glm::rotate(float(pi * chisl / znam), glm::vec3(0.0, 1.0, 0.0));
        glm::mat4 mv = glm::lookAt(glm::vec3(-5, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)) * scale * translate * rotateX * rotateY;
        glm::mat4 mvp = glm::perspective(float(pi / 6.0), float(4.0 / 3.0), 0.1f, 100.0f) * mv;

        glUniformMatrix4fv(g_MV, 1, GL_FALSE, &mv[0][0]);
        glUniformMatrix4fv(g_MVP, 1, GL_FALSE, &mvp[0][0]);
        glDrawElements(GL_TRIANGLES, g_modelRotate.indexCount, GL_UNSIGNED_INT, NULL);
    }
}

void cleanup(Model g_model)
{
    if (g_model.vbo != 0)
        glDeleteBuffers(1, &g_model.vbo);
    if (g_model.ibo != 0)
        glDeleteBuffers(1, &g_model.ibo);
    if (g_model.vao != 0)
        glDeleteVertexArrays(1, &g_model.vao);
}

bool initOpenGL()
{
    // Initialize GLFW functions.
    if (!glfwInit())
    {
        cout << "Failed to initialize GLFW" << endl;
        return false;
    }

    // Request OpenGL 3.3 without obsoleted functions.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window.
    g_window = glfwCreateWindow(800, 600, "OpenGL Test", NULL, NULL);
    if (g_window == NULL)
    {
        cout << "Failed to open GLFW window" << endl;
        glfwTerminate();
        return false;
    }

    // Initialize OpenGL context with.
    glfwMakeContextCurrent(g_window);

    // Set internal GLEW variable to activate OpenGL core profile.
    glewExperimental = true;

    // Initialize GLEW functions.
    if (glewInit() != GLEW_OK)
    {
        cout << "Failed to initialize GLEW" << endl;
        return false;
    }

    // Ensure we can capture the escape key being pressed.
    glfwSetInputMode(g_window, GLFW_STICKY_KEYS, GL_TRUE);

    // Set callback for framebuffer resizing event.
    glfwSetFramebufferSizeCallback(g_window, reshape);

    return true;
}

void tearDownOpenGL()
{
    // Terminate GLFW.
    glfwTerminate();
}

bool tbezierSO1(const vector<point>& values, vector<Segment>& curve)
{
    int n = values.size() - 1;
    if (n < 1) return false;
    curve.resize(n);

    point cur, next, tgL, tgR, deltaL, deltaC, deltaR;
    double l1, l2;

    next = values[1] - values[0];
    next.normalize();

    for (int i = 0; i < n; ++i)
    {
        tgL = tgR;
        cur = next;

        deltaC = values[i + 1] - values[i];

        if (i > 0) deltaL = point::absMin(deltaC, values[i] - values[i - 1]);
        else deltaL = deltaC;

        if (i < n - 1)
        {
            next = values[i + 2] - values[i + 1];
            next.normalize();
            if (IS_ZERO(cur.x) || IS_ZERO(cur.y)) tgR = cur;
            else if (IS_ZERO(next.x) || IS_ZERO(next.y)) tgR = next;
            else tgR = cur + next;
            tgR.normalize();
            deltaR = point::absMin(deltaC, values[i + 2] - values[i + 1]);
        }
        else
        {
            tgR = point();
            deltaR = deltaC;
        }

        l1 = IS_ZERO(tgL.x) ? 0.0 : deltaL.x / (C * tgL.x);
        l2 = IS_ZERO(tgR.x) ? 0.0 : deltaR.x / (C * tgR.x);

        if (abs(l1 * tgL.y) > abs(deltaL.y))
            l1 = IS_ZERO(tgL.y) ? 0.0 : deltaL.y / tgL.y;
        if (abs(l2 * tgR.y) > abs(deltaR.y))
            l2 = IS_ZERO(tgR.y) ? 0.0 : deltaR.y / tgR.y;

        curve[i].points[0] = values[i];
        curve[i].points[1] = curve[i].points[0] + tgL * l1;
        curve[i].points[3] = values[i + 1];
        curve[i].points[2] = curve[i].points[3] - tgR * l2;
    }

    return true;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        double xpos, ypos;

        glfwGetCursorPos(g_window, &xpos, &ypos);
        glfwGetWindowSize(g_window, &w, &h);

        xpos = xpos / w * 2. - 1.;
        ypos = 1. - ypos / h * 2.;

        points.emplace_back(xpos, ypos);

        bezierPoints.clear();
        vector<Segment> curve;
        tbezierSO1(points, curve);

        for (auto s : curve)
        {
            for (int i = 0; i < RESOLUTION; ++i)
            {
                point p = s.calc((double)i / (double)RESOLUTION);
                bezierPoints.push_back(p);
            }
        }

        createModelPoint();
        createModelLine();
    }
}

int main()
{
    // Initialize OpenGL
    if (!initOpenGL())
        return -1;

    // Initialize graphical resources.
    bool isOk = init();

    glfwSetMouseButtonCallback(g_window, mouse_button_callback); //включение отклика на мышь
    bool mode = false;
    int chisl = 0;

    if (isOk)
    {
        // Main loop until window closed or escape pressed.
        while (glfwGetKey(g_window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(g_window) == 0)
        {
            if (glfwGetKey(g_window, GLFW_KEY_SPACE) == GLFW_PRESS && !mode)
            {
                glfwSetMouseButtonCallback(g_window, NULL);

                GLuint vertexShader, fragmentShader;
                vertexShader = createShader(vshR, GL_VERTEX_SHADER);
                fragmentShader = createShader(fshR, GL_FRAGMENT_SHADER);

                g_shaderProgramRotate = createProgram(vertexShader, fragmentShader);
                g_MVP = glGetUniformLocation(g_shaderProgramRotate, "u_mvp");
                g_MV = glGetUniformLocation(g_shaderProgramRotate, "u_mv");

                glDeleteShader(vertexShader);
                glDeleteShader(fragmentShader);
                
                createModelRotate();
                mode = true;
            }

            // Draw scene.
            draw(mode, chisl);
            if (mode) chisl++;
            if (chisl == 2 * znam) chisl = 0;
            // Swap buffers.
            glfwSwapBuffers(g_window);
            // Poll window events.
            glfwPollEvents();
        }
    }

    // Cleanup graphical resources.
    cleanup(g_modelLine);
    cleanup(g_modelPoint);
    cleanup(g_modelRotate);

    // Tear down OpenGL.
    tearDownOpenGL();

    return isOk ? 0 : -1;
}