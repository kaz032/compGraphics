#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Window configuration
const unsigned int SCREEN_WIDTH = 800;
const unsigned int SCREEN_HEIGHT = 600;

// Camera and lighting state
float cameraOrbitAngle = 0.0f;
float currentCameraHeight = 2.0f;
bool isLightEnabled = true;
bool shouldUseMagentaMaterial = false;
float lightOrbitAngle = 0.0f;

// OpenGL resources
GLuint mainShaderProgram;
GLuint cubeVertexArray, cubeVertexBuffer, sphereVertexArray, sphereVertexBuffer;
GLuint sunTextureID;
int sphereTotalVertices;

// Vertex Shader
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 fragmentPosition;
out vec3 surfaceNormal;
out vec2 textureCoordinates;

void main() {
    fragmentPosition = vec3(modelMatrix * vec4(position, 1.0));
    surfaceNormal = mat3(transpose(inverse(modelMatrix))) * normal;
    textureCoordinates = texCoord;
    gl_Position = projectionMatrix * viewMatrix * vec4(fragmentPosition, 1.0);
}
)";

// Fragment Shader
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 fragmentPosition;
in vec3 surfaceNormal;
in vec2 textureCoordinates;

out vec4 finalColor;

uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform bool isLightEnabled;
uniform bool shouldUseMagentaMaterial;
uniform bool isRenderingSphere;
uniform sampler2D surfaceTexture;

void main() {
    if (isRenderingSphere) {
        finalColor = texture(surfaceTexture, textureCoordinates);
        return;
    }

    vec3 baseColor = shouldUseMagentaMaterial ? vec3(1.0, 0.0, 1.0) : vec3(1.0);
    vec3 normalizedNormal = normalize(surfaceNormal);
    vec3 lightDirection = normalize(lightPosition - fragmentPosition);
    float diffuseStrength = max(dot(normalizedNormal, lightDirection), 0.0);

    vec3 diffuseLight = isLightEnabled ? diffuseStrength * lightColor : vec3(0.0);
    vec3 combinedColor = (diffuseLight + 0.2) * baseColor;
    finalColor = vec4(combinedColor, 1.0);
}
)";

// Cube vertex data
float cubeVertexData[] = {
    // Positions  
    -0.5,-0.5,-0.5,  0, 0,-1,  0,0,
     0.5,-0.5,-0.5,  0, 0,-1,  1,0,
     0.5, 0.5,-0.5,  0, 0,-1,  1,1,
     0.5, 0.5,-0.5,  0, 0,-1,  1,1,
    -0.5, 0.5,-0.5,  0, 0,-1,  0,1,
    -0.5,-0.5,-0.5,  0, 0,-1,  0,0,

    -0.5,-0.5, 0.5,  0, 0, 1,  0,0,
     0.5,-0.5, 0.5,  0, 0, 1,  1,0,
     0.5, 0.5, 0.5,  0, 0, 1,  1,1,
     0.5, 0.5, 0.5,  0, 0, 1,  1,1,
    -0.5, 0.5, 0.5,  0, 0, 1,  0,1,
    -0.5,-0.5, 0.5,  0, 0, 1,  0,0,

    -0.5, 0.5, 0.5, -1, 0, 0,  1,0,
    -0.5, 0.5,-0.5, -1, 0, 0,  1,1,
    -0.5,-0.5,-0.5, -1, 0, 0,  0,1,
    -0.5,-0.5,-0.5, -1, 0, 0,  0,1,
    -0.5,-0.5, 0.5, -1, 0, 0,  0,0,
    -0.5, 0.5, 0.5, -1, 0, 0,  1,0,

     0.5, 0.5, 0.5,  1, 0, 0,  1,0,
     0.5, 0.5,-0.5,  1, 0, 0,  1,1,
     0.5,-0.5,-0.5,  1, 0, 0,  0,1,
     0.5,-0.5,-0.5,  1, 0, 0,  0,1,
     0.5,-0.5, 0.5,  1, 0, 0,  0,0,
     0.5, 0.5, 0.5,  1, 0, 0,  1,0,

    -0.5,-0.5,-0.5,  0,-1, 0,  0,1,
     0.5,-0.5,-0.5,  0,-1, 0,  1,1,
     0.5,-0.5, 0.5,  0,-1, 0,  1,0,
     0.5,-0.5, 0.5,  0,-1, 0,  1,0,
    -0.5,-0.5, 0.5,  0,-1, 0,  0,0,
    -0.5,-0.5,-0.5,  0,-1, 0,  0,1,

    -0.5, 0.5,-0.5,  0, 1, 0,  0,1,
     0.5, 0.5,-0.5,  0, 1, 0,  1,1,
     0.5, 0.5, 0.5,  0, 1, 0,  1,0,
     0.5, 0.5, 0.5,  0, 1, 0,  1,0,
    -0.5, 0.5, 0.5,  0, 1, 0,  0,0,
    -0.5, 0.5,-0.5,  0, 1, 0,  0,1,
};

// Matrix operations
void createIdentityMatrix(float* matrix) {
    for (int i = 0; i < 16; ++i) {
        matrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
}

void createTranslationMatrix(float* matrix, float x, float y, float z) {
    createIdentityMatrix(matrix);
    matrix[12] = x;
    matrix[13] = y;
    matrix[14] = z;
}

void multiplyMatrices(float* result, const float* first, const float* second) {
    float temp[16] = { 0 };
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            for (int k = 0; k < 4; ++k) {
                temp[row * 4 + col] += first[row * 4 + k] * second[k * 4 + col];
            }
        }
    }
    memcpy(result, temp, 16 * sizeof(float));
}

void createScalingMatrix(float* matrix, float x, float y, float z) {
    createIdentityMatrix(matrix);
    matrix[0] = x;
    matrix[5] = y;
    matrix[10] = z;
}

void createProjectionMatrix(float* matrix, float fov, float aspectRatio, float nearPlane, float farPlane) {
    float tanHalfFOV = 1.0f / tanf(fov / 2.0f);
    for (int i = 0; i < 16; ++i) matrix[i] = 0.0f;
    matrix[0] = tanHalfFOV / aspectRatio;
    matrix[5] = tanHalfFOV;
    matrix[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    matrix[11] = -1.0f;
    matrix[14] = (2 * farPlane * nearPlane) / (nearPlane - farPlane);
}

void createViewMatrix(float* matrix, float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ) {
    float forward[3] = { centerX - eyeX, centerY - eyeY, centerZ - eyeZ };
    float forwardLength = sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
    for (int i = 0; i < 3; ++i) forward[i] /= forwardLength;

    float up[3] = { upX, upY, upZ };
    float right[3] = {
        forward[1] * up[2] - forward[2] * up[1],
        forward[2] * up[0] - forward[0] * up[2],
        forward[0] * up[1] - forward[1] * up[0]
    };
    float rightLength = sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
    for (int i = 0; i < 3; ++i) right[i] /= rightLength;

    float correctedUp[3] = {
        right[1] * forward[2] - right[2] * forward[1],
        right[2] * forward[0] - right[0] * forward[2],
        right[0] * forward[1] - right[1] * forward[0]
    };

    createIdentityMatrix(matrix);
    matrix[0] = right[0]; matrix[4] = right[1]; matrix[8] = right[2];
    matrix[1] = correctedUp[0]; matrix[5] = correctedUp[1]; matrix[9] = correctedUp[2];
    matrix[2] = -forward[0]; matrix[6] = -forward[1]; matrix[10] = -forward[2];
    matrix[12] = -right[0] * eyeX - right[1] * eyeY - right[2] * eyeZ;
    matrix[13] = -correctedUp[0] * eyeX - correctedUp[1] * eyeY - correctedUp[2] * eyeZ;
    matrix[14] = forward[0] * eyeX + forward[1] * eyeY + forward[2] * eyeZ;
}

// Shader management
GLuint compileGLShader(GLenum shaderType, const char* sourceCode) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &sourceCode, NULL);
    glCompileShader(shader);

    GLint successStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &successStatus);
    if (!successStatus) {
        char errorLog[512];
        glGetShaderInfoLog(shader, 512, NULL, errorLog);
        std::cerr << "Shader compilation failed:\n" << errorLog << std::endl;
    }
    return shader;
}

void initializeShaderProgram() {
    GLuint vertexShader = compileGLShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileGLShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    mainShaderProgram = glCreateProgram();
    glAttachShader(mainShaderProgram, vertexShader);
    glAttachShader(mainShaderProgram, fragmentShader);
    glLinkProgram(mainShaderProgram);

    GLint linkStatus;
    glGetProgramiv(mainShaderProgram, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        char errorLog[512];
        glGetProgramInfoLog(mainShaderProgram, 512, NULL, errorLog);
        std::cerr << "Shader program linking failed:\n" << errorLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

// Texture handling
GLuint loadTextureFromFile(const char* filePath) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* imageData = stbi_load(filePath, &width, &height, &channels, 0);
    if (!imageData) {
        std::cerr << "Failed to load texture: " << filePath << std::endl;
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(imageData);
    return texture;
}

// Geometry creation
void appendVertexData(std::vector<float>& vertexBuffer, float x, float y, float z,
    float nx, float ny, float nz, float s, float t) {
    vertexBuffer.insert(vertexBuffer.end(), { x, y, z, nx, ny, nz, s, t });
}

void generateSphereGeometry(float radius, int longitudeSegments, int latitudeSegments,
    std::vector<float>& vertexBuffer) {
    const float PI = 3.1415926f;
    std::vector<float> tempVertices;

    float longitudeStep = 2 * PI / longitudeSegments;
    float latitudeStep = PI / latitudeSegments;

    for (int latitude = 0; latitude <= latitudeSegments; ++latitude) {
        float verticalAngle = PI / 2 - latitude * latitudeStep;
        float xy = radius * cosf(verticalAngle);
        float z = radius * sinf(verticalAngle);

        for (int longitude = 0; longitude <= longitudeSegments; ++longitude) {
            float horizontalAngle = longitude * longitudeStep;
            float x = xy * cosf(horizontalAngle);
            float y = xy * sinf(horizontalAngle);

            float normalX = x / radius;
            float normalY = y / radius;
            float normalZ = z / radius;

            float u = (float)longitude / longitudeSegments;
            float v = (float)latitude / latitudeSegments;

            appendVertexData(tempVertices, x, y, z, normalX, normalY, normalZ, u, v);
        }
    }

    std::vector<unsigned int> indices;
    for (int latitude = 0; latitude < latitudeSegments; ++latitude) {
        int currentRow = latitude * (longitudeSegments + 1);
        int nextRow = currentRow + longitudeSegments + 1;

        for (int longitude = 0; longitude < longitudeSegments; ++longitude) {

            if (latitude != 0) {
                indices.insert(indices.end(), {
                    static_cast<unsigned int>(currentRow),
                    static_cast<unsigned int>(nextRow),
                    static_cast<unsigned int>(currentRow + 1)
                    });
            }
            if (latitude != (latitudeSegments - 1)) {
                indices.insert(indices.end(), {
                    static_cast<unsigned int>(currentRow + 1),
                    static_cast<unsigned int>(nextRow),
                    static_cast<unsigned int>(nextRow + 1)
                    });
            }
            currentRow++;
            nextRow++;
        }
    }

    std::vector<float> finalVertices;
    for (auto index : indices) {
        finalVertices.insert(finalVertices.end(),
            tempVertices.begin() + index * 8,
            tempVertices.begin() + (index + 1) * 8);
    }
    vertexBuffer = finalVertices;
}

void initializeCubeGeometry() {
    glGenVertexArrays(1, &cubeVertexArray);
    glGenBuffers(1, &cubeVertexBuffer);

    glBindVertexArray(cubeVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertexData), cubeVertexData, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void initializeSphereGeometry() {
    std::vector<float> sphereVertices;
    generateSphereGeometry(0.5f, 36, 18, sphereVertices);
    sphereTotalVertices = sphereVertices.size() / 8;

    glGenVertexArrays(1, &sphereVertexArray);
    glGenBuffers(1, &sphereVertexBuffer);

    glBindVertexArray(sphereVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

// Input handling
void handleKeyboardInput(GLFWwindow* window, int key, int, int action, int) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_LEFT:  cameraOrbitAngle -= 0.1f; break;
        case GLFW_KEY_RIGHT: cameraOrbitAngle += 0.1f; break;
        case GLFW_KEY_L:     isLightEnabled = !isLightEnabled; break;
        case GLFW_KEY_M:     shouldUseMagentaMaterial = !shouldUseMagentaMaterial; break;
        }
    }
}

// Main application
int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* mainWindow = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "3D Scene Viewer", NULL, NULL);
    if (!mainWindow) {
        std::cerr << "Window creation failed" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(mainWindow);
    glfwSetKeyCallback(mainWindow, handleKeyboardInput);

    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed" << std::endl;
        return -1;
    }

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    initializeShaderProgram();
    initializeCubeGeometry();
    initializeSphereGeometry();
    sunTextureID = loadTextureFromFile("sun.jpg");

    if (sunTextureID == 0) {
        std::cerr << "Texture loading failed - using fallback colors" << std::endl;
    }

    while (!glfwWindowShouldClose(mainWindow)) {
        glfwPollEvents();

        // Frame setup
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(mainShaderProgram);

        // Camera setup
        float viewMatrix[16], projectionMatrix[16];
        float cameraX = 6.0f * cos(cameraOrbitAngle);
        float cameraZ = 6.0f * sin(cameraOrbitAngle);
        createViewMatrix(viewMatrix, cameraX, currentCameraHeight, cameraZ, 0, 0, 0, 0, 1, 0);
        createProjectionMatrix(projectionMatrix, 3.1415f / 4,
            (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "viewMatrix"),
            1, GL_FALSE, viewMatrix);
        glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "projectionMatrix"),
            1, GL_FALSE, projectionMatrix);

        // Light animation
        lightOrbitAngle += 0.005f;
        float lightX = 4.0f * cos(lightOrbitAngle);
        float lightZ = 4.0f * sin(lightOrbitAngle);
        glUniform3f(glGetUniformLocation(mainShaderProgram, "lightPosition"),
            lightX, 2.0f, lightZ);
        glUniform3f(glGetUniformLocation(mainShaderProgram, "lightColor"), 1.0f, 1.0f, 0.0f);
        glUniform1i(glGetUniformLocation(mainShaderProgram, "isLightEnabled"), isLightEnabled);
        glUniform1i(glGetUniformLocation(mainShaderProgram, "shouldUseMagentaMaterial"),
            shouldUseMagentaMaterial);

        // Render cubes
        glBindVertexArray(cubeVertexArray);
        glUniform1i(glGetUniformLocation(mainShaderProgram, "isRenderingSphere"), 0);
        for (int i = -1; i <= 1; ++i) {
            float modelMatrix[16];
            createTranslationMatrix(modelMatrix, i * 1.1f, 0, 0);
            glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "modelMatrix"),
                1, GL_FALSE, modelMatrix);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Render sphere 
        glBindVertexArray(sphereVertexArray);
        glUniform1i(glGetUniformLocation(mainShaderProgram, "isRenderingSphere"), 1);
        glBindTexture(GL_TEXTURE_2D, sunTextureID);

        float sphereModelMatrix[16], scaleMatrix[16];
        createTranslationMatrix(sphereModelMatrix, lightX, 1.8f, lightZ);
        createScalingMatrix(scaleMatrix, 0.5f, 0.5f, 0.5f);
        multiplyMatrices(sphereModelMatrix, sphereModelMatrix, scaleMatrix);

        glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "modelMatrix"),
            1, GL_FALSE, sphereModelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, sphereTotalVertices);

        glfwSwapBuffers(mainWindow);
    }


    glDeleteVertexArrays(1, &cubeVertexArray);
    glDeleteVertexArrays(1, &sphereVertexArray);
    glDeleteBuffers(1, &cubeVertexBuffer);
    glDeleteBuffers(1, &sphereVertexBuffer);
    glDeleteProgram(mainShaderProgram);
    glDeleteTextures(1, &sunTextureID);

    glfwTerminate();
    return 0;
}