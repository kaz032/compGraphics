#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>

const int WIN_W = 800;
const int WIN_H = 800;
const float PT_RADIUS = 0.05f;

struct BZpoint {
    float x, y;
    BZpoint mult(float s) const { return { x * s, y * s }; }
    BZpoint add(BZpoint p) const { return { x + p.x, y + p.y }; }
    float dist(BZpoint p) const {
        return std::sqrt((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y));
    }
};

std::vector<BZpoint> pts = {
    {-0.7f, -0.3f},
    {-0.3f, 0.8f},
    {0.1f, -0.5f},
    {0.5f, 0.3f}
};
int activeIdx = -1;

GLuint shaderProg;
GLuint vao[3], vbo[3];

BZpoint bezier(float t, const std::vector<BZpoint>& p) {
    std::vector<BZpoint> tmp = p;
    for (int k = 1; k < tmp.size(); ++k)
        for (int i = 0; i < tmp.size() - k; ++i)
            tmp[i] = tmp[i].mult(1 - t).add(tmp[i + 1].mult(t));
    return tmp[0];
}

void updateBuffers() {
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(BZpoint), pts.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(BZpoint), pts.data(), GL_DYNAMIC_DRAW);

    if (pts.size() >= 2) {
        std::vector<BZpoint> curve;
        for (float t = 0; t <= 1.0f; t += 0.001f)
            curve.push_back(bezier(t, pts));
        glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
        glBufferData(GL_ARRAY_BUFFER, curve.size() * sizeof(BZpoint), curve.data(), GL_DYNAMIC_DRAW);
    }
}

void mouseBtn(GLFWwindow* win, int btn, int act, int mods) {
    double mx, my;
    glfwGetCursorPos(win, &mx, &my);
    BZpoint mouse = {
        float(mx / WIN_W * 2 - 1),
        float(1 - my / WIN_H * 2)
    };

    if (btn == GLFW_MOUSE_BUTTON_LEFT && act == GLFW_PRESS) {
        for (int i = 0; i < pts.size(); ++i) {
            if (pts[i].dist(mouse) < PT_RADIUS) {
                activeIdx = i;
                return;
            }
        }
        pts.push_back(mouse);
        updateBuffers();
    }
    else if (btn == GLFW_MOUSE_BUTTON_RIGHT && act == GLFW_PRESS) {
        for (int i = 0; i < pts.size(); ++i) {
            if (pts[i].dist(mouse) < PT_RADIUS) {
                pts.erase(pts.begin() + i);
                updateBuffers();
                return;
            }
        }
    }
    else if (btn == GLFW_MOUSE_BUTTON_LEFT && act == GLFW_RELEASE) {
        activeIdx = -1;
    }
}

void mouseMove(GLFWwindow* win, double x, double y) {
    if (activeIdx >= 0) {
        pts[activeIdx] = {
            float(x / WIN_W * 2 - 1),
            float(1 - y / WIN_H * 2)
        };
        updateBuffers();
    }
}

const char* vertShader = R"(
#version 330
layout(location=0) in vec2 pos;
void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

const char* fragShader = R"(
#version 330
out vec4 color;
uniform vec3 col;
void main() {
    color = vec4(col, 1.0);
}
)";

void initShaders() {
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertShader, NULL);
    glCompileShader(vert);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragShader, NULL);
    glCompileShader(frag);

    shaderProg = glCreateProgram();
    glAttachShader(shaderProg, vert);
    glAttachShader(shaderProg, frag);
    glLinkProgram(shaderProg);

    glDeleteShader(vert);
    glDeleteShader(frag);
}

void initGL() {
    glGenVertexArrays(3, vao);
    glGenBuffers(3, vbo);
    for (int i = 0; i < 3; ++i) {
        glBindVertexArray(vao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[i]);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);
    }
}

int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* win = glfwCreateWindow(WIN_W, WIN_H, "Bezier", NULL, NULL);
    if (!win) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(win);
    glewInit();

    initShaders();
    initGL();
    updateBuffers();

    glfwSetMouseButtonCallback(win, mouseBtn);
    glfwSetCursorPosCallback(win, mouseMove);

    while (!glfwWindowShouldClose(win)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProg);

        glUniform3f(glGetUniformLocation(shaderProg, "col"), 1.0f, 0.0f, 0.0f);
        glBindVertexArray(vao[0]);
        glPointSize(10.0f);
        glDrawArrays(GL_POINTS, 0, pts.size());

        glUniform3f(glGetUniformLocation(shaderProg, "col"), 0.0f, 0.0f, 1.0f);
        glBindVertexArray(vao[1]);
        glDrawArrays(GL_LINE_STRIP, 0, pts.size());

        if (pts.size() >= 2) {
            glUniform3f(glGetUniformLocation(shaderProg, "col"), 0.0f, 1.0f, 0.0f);
            glBindVertexArray(vao[2]);
            glDrawArrays(GL_LINE_STRIP, 0, 1001);
        }

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}