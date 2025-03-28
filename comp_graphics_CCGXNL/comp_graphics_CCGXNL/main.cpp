#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#ifndef MATH_PI
#define MATH_PI 3.14159265358979323846
#endif

const int SCREEN_MEASURE = 700;
const float BALL_SPEED = 0.0004f;
const float PADDLE_SPEED = 0.0005f;
const float RADIANS = MATH_PI / 180.0f;
const float BALL_RADIUS = 0.1f;
const float PADDLE_SIZE = 0.25f;

float ballXcoord = 0.0f, ballYcoord = 0.0f;
float ballXmove = cos(35.0f * RADIANS), ballYmove = sin(35.0f * RADIANS);
float paddlePosition = 0.0f;
bool gameRunning = false;
bool colorFlip = false;

const char* ballVertShader = R"(
    #version 330 core
    layout(location = 0) in vec2 vertexPos;
    uniform vec2 ballPosition;
    void main() {
        gl_Position = vec4(vertexPos + ballPosition, 0.0, 1.0);
    }
)";

const char* ballFragShader = R"(
    #version 330 core
    out vec4 finalColor;
    uniform vec2 ballCenter;
    uniform float ballSize;
    uniform bool flipColors;
    
    void main() {
        vec2 pixelPos = gl_FragCoord.xy / 700.0 * 2.0 - 1.0;
        float dist = distance(pixelPos, ballCenter);
        float colorMix = smoothstep(ballSize, 0.0, dist);
        
        if(flipColors) {
            finalColor = mix(vec4(1.0, 0.0, 0.0, 1.0), 
                           vec4(0.0, 1.0, 0.0, 1.0), 
                           colorMix);
        } else {
            finalColor = mix(vec4(0.0, 1.0, 0.0, 1.0), 
                           vec4(1.0, 0.0, 0.0, 1.0), 
                           colorMix);
        }
    }
)";

const char* paddleVertShader = R"(
    #version 330 core
    layout(location = 0) in vec2 vertexPos;
    void main() {
        gl_Position = vec4(vertexPos, 0.0, 1.0);
    }
)";

const char* paddleFragShader = R"(
    #version 330 core
    out vec4 finalColor;
    void main() {
        finalColor = vec4(0.0, 0.0, 1.0, 1.0);
    }
)";

void handleControls(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gameRunning = true;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        paddlePosition += PADDLE_SPEED;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        paddlePosition -= PADDLE_SPEED;
}

void updateBall() {
    if (!gameRunning) return;

    ballXcoord += ballXmove * BALL_SPEED;
    ballYcoord += ballYmove * BALL_SPEED;

    if (ballXcoord > 0.9f || ballXcoord < -0.9f) ballXmove = -ballXmove;
    if (ballYcoord > 0.9f || ballYcoord < -0.9f) ballYmove = -ballYmove;

    bool yCollision = fabs(ballYcoord - paddlePosition) <= BALL_RADIUS;
    bool xCollision = (ballXcoord >= -PADDLE_SIZE / 2 - BALL_RADIUS) &&
        (ballXcoord <= PADDLE_SIZE / 2 + BALL_RADIUS);
    colorFlip = yCollision && xCollision;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* gameWindow = glfwCreateWindow(SCREEN_MEASURE, SCREEN_MEASURE, "Ball Game", NULL, NULL);
    if (!gameWindow) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(gameWindow);
    glewInit();

    GLuint ballVAO, paddleVAO;
    GLuint ballVBO, paddleVBO;
    glGenVertexArrays(1, &ballVAO);
    glGenVertexArrays(1, &paddleVAO);
    glGenBuffers(1, &ballVBO);
    glGenBuffers(1, &paddleVBO);

    float ballPoints[362 * 2];
    ballPoints[0] = 0.0f; ballPoints[1] = 0.0f;
    for (int i = 0; i <= 360; ++i) {
        float angle = i * MATH_PI / 180.0f;
        ballPoints[2 * i + 2] = cos(angle) * BALL_RADIUS;
        ballPoints[2 * i + 3] = sin(angle) * BALL_RADIUS;
    }

    float paddlePoints[] = {
        -PADDLE_SIZE / 2, paddlePosition,
        PADDLE_SIZE / 2, paddlePosition
    };

    glBindVertexArray(ballVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ballVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ballPoints), ballPoints, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(paddleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, paddleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(paddlePoints), paddlePoints, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    GLuint ballShader = glCreateProgram();
    GLuint ballVert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(ballVert, 1, &ballVertShader, NULL);
    glCompileShader(ballVert);
    GLuint ballFrag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(ballFrag, 1, &ballFragShader, NULL);
    glCompileShader(ballFrag);
    glAttachShader(ballShader, ballVert);
    glAttachShader(ballShader, ballFrag);
    glLinkProgram(ballShader);

    GLuint paddleShader = glCreateProgram();
    GLuint paddleVert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(paddleVert, 1, &paddleVertShader, NULL);
    glCompileShader(paddleVert);
    GLuint paddleFrag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(paddleFrag, 1, &paddleFragShader, NULL);
    glCompileShader(paddleFrag);
    glAttachShader(paddleShader, paddleVert);
    glAttachShader(paddleShader, paddleFrag);
    glLinkProgram(paddleShader);

    while (!glfwWindowShouldClose(gameWindow)) {
        glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        handleControls(gameWindow);
        updateBall();

        paddlePoints[1] = paddlePosition;
        paddlePoints[3] = paddlePosition;
        glBindBuffer(GL_ARRAY_BUFFER, paddleVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(paddlePoints), paddlePoints, GL_DYNAMIC_DRAW);

        glUseProgram(ballShader);
        glBindVertexArray(ballVAO);
        glUniform2f(glGetUniformLocation(ballShader, "ballPosition"), ballXcoord, ballYcoord);
        glUniform2f(glGetUniformLocation(ballShader, "ballCenter"), ballXcoord, ballYcoord);
        glUniform1f(glGetUniformLocation(ballShader, "ballSize"), BALL_RADIUS);
        glUniform1i(glGetUniformLocation(ballShader, "flipColors"), colorFlip);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 362);

        glUseProgram(paddleShader);
        glBindVertexArray(paddleVAO);
        glDrawArrays(GL_LINES, 0, 2);

        glfwSwapBuffers(gameWindow);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &ballVBO);
    glDeleteBuffers(1, &paddleVBO);
    glDeleteVertexArrays(1, &ballVAO);
    glDeleteVertexArrays(1, &paddleVAO);
    glDeleteProgram(ballShader);
    glDeleteProgram(paddleShader);

    glfwTerminate();
    return 0;
}