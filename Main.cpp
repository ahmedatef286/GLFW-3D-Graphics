#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <ctime>
#include <random>

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

float birdX = 50.0f;
float birdY = 250.0f;
float gravity = -1000;

float jumpVelocity = 500.f;
float birdYAcceleration = 0.f;
float terminalVelocity = 250.f;
float jumpTimer = 0.1;
float timeSinceLastJump = 0.0;
float birdRadius = 15;

float pipeWidth = 50.0f;
float pipeVelocity = 200.0f;
bool gameOver = false;
float deltaTime = 0.0f;
float lastFrameTime = 0.0f;

std::vector<std::pair<float, std::pair<float, float>>> pipes;
float pipeSpawnTimer = 0.0f;
float minPipeSpawnInterval = 1.0f;
float maxPipeSpawnInterval = 4.0f;
float pipeGap = 100.0f;
float minHeight = 50;
int heightRange = 200;
float minPosition = 50;
int positionRange = 200;
float prevGapPosition = 0.0f;

GLuint birdTexture, pipeTexture, birdShaderProgram, pipeShaderProgram;
int birdTextureWidth, birdTextureHeight;
int pipeTextureWidth, pipeTextureHeight;


GLuint backgroundTexture, backgroundShaderProgram;
int backgroundTextureWidth, backgroundTextureHeight;

GLuint birdVAO, birdVBO, pipeVAO, pipeVBO;

std::default_random_engine generator(time(0));

void initialize(GLFWwindow* window);
void update(GLFWwindow* window);
void drawBird();
void drawBackground();
void drawPipe(float x, float upperPipeHeight, float gapPosition);
void checkCollision(GLFWwindow* window);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
GLuint loadTexture(const char* path, int& width, int& height);
void setupVertexAttributes();

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Flappy Bird", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create GLFW window" << std::endl;
        return -1;
    }

    initialize(window);

    glfwSetKeyCallback(window, keyCallback);

    lastFrameTime = glfwGetTime();

    birdTexture = loadTexture("textures/flappy-bird-bird.png", birdTextureWidth, birdTextureHeight);
    assert(birdTexture != 0);

    pipeTexture = loadTexture("textures/flappy-bird-lower-pipe.png", pipeTextureWidth, pipeTextureHeight);
    assert(pipeTexture != 0);

    backgroundTexture = loadTexture("textures/flappy-bird-background.png", backgroundTextureWidth, backgroundTextureHeight);
    assert(backgroundTexture != 0);

    glfwSwapInterval(1); // Enable V-Sync

    while (!glfwWindowShouldClose(window)) {
        float currentFrameTime = glfwGetTime();
        deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;
        timeSinceLastJump += deltaTime;

        update(window);
        glClear(GL_COLOR_BUFFER_BIT);
        drawBackground();


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, birdTexture);
        drawBird();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pipeTexture);
        for (const auto& pipe : pipes) {
            drawPipe(pipe.first, pipe.second.first, pipe.second.second);
        }

        glBindTexture(GL_TEXTURE_2D, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void initialize(GLFWwindow* window) {
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        exit(EXIT_FAILURE);
    }

    glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);




    // Bird vertex shader source code
    const char* birdVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        uniform float translationY;
        void main() {
            gl_Position = vec4(aPos.x - 0.25, aPos.y + translationY, 0.0, 0.5);
            TexCoord = aTexCoord;
        }
    )";

    // Pipe vertex shader source code
    const char* pipeVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        uniform float translationX;
        uniform float translationY;
        uniform int isLowerPart;
        void main() {
            gl_Position = vec4(aPos.x + translationX + 0.25, aPos.y + 0.25 - translationY, 0.0, 0.5);
            TexCoord = isLowerPart == 1 ? aTexCoord : vec2(aTexCoord.x, 1.0 - aTexCoord.y);
        }
        )";
    //fragment shader source code
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D ourTexture;
        void main() {
            FragColor = texture(ourTexture, TexCoord);
        }
    )";


    const char* backgroundVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    void main() {
        gl_Position = vec4(aPos, 0.0, 0.02);
        TexCoord = aTexCoord;
    }
)";
    GLuint backgroundVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(backgroundVertexShader, 1, &backgroundVertexShaderSource, nullptr);
    glCompileShader(backgroundVertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    backgroundShaderProgram = glCreateProgram();
    glAttachShader(backgroundShaderProgram, backgroundVertexShader);
    glAttachShader(backgroundShaderProgram, fragmentShader);
    glLinkProgram(backgroundShaderProgram);



    GLuint birdVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(birdVertexShader, 1, &birdVertexShaderSource, nullptr);
    glCompileShader(birdVertexShader);

    GLuint pipeVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(pipeVertexShader, 1, &pipeVertexShaderSource, nullptr);
    glCompileShader(pipeVertexShader);

    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    birdShaderProgram = glCreateProgram();
    glAttachShader(birdShaderProgram, birdVertexShader);
    glAttachShader(birdShaderProgram, fragmentShader);
    glLinkProgram(birdShaderProgram);

    pipeShaderProgram = glCreateProgram();
    glAttachShader(pipeShaderProgram, pipeVertexShader);
    glAttachShader(pipeShaderProgram, fragmentShader);
    glLinkProgram(pipeShaderProgram);

    glUseProgram(birdShaderProgram);

    int textureUniform = glGetUniformLocation(birdShaderProgram, "ourTexture");
    glUniform1i(textureUniform, 0);

    glGenVertexArrays(1, &birdVAO);
    glGenBuffers(1, &birdVBO);

    glGenVertexArrays(1, &pipeVAO);
    glGenBuffers(1, &pipeVBO);

    setupVertexAttributes();
}

void setupVertexAttributes() {
    // Setup bird VAO and VBO
    glBindVertexArray(birdVAO);
    glBindBuffer(GL_ARRAY_BUFFER, birdVBO);
    float birdVertices[] = {
        -(17.0f / 800.0f), (12.0f / 600.0f), 0.0f, 0.0f,
        (17.0f / 800.0f), (12.0f / 600.0f), 1.0f, 0.0f,
        (17.0f / 800.0f), -(12.0f / 600.0f), 1.0f, 1.0f,

        (17.0f / 800.0f), -(12.0f / 600.0f), 1.0f, 1.0f,
        -(17.0f / 800.0f), -(12.0f / 600.0f), 0.0f, 1.0f,
        -(17.0f / 800.0f), (12.0f / 600.0f), 0.0f, 0.0f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(birdVertices), birdVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Setup pipe VAO and VBO
    glBindVertexArray(pipeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pipeVBO);
    float pipeVertices[] = {
        -(26.0f / 800.0f), -(154.0f / 600.0f), 0.0f, 0.0f,
        (26.0f / 800.0f), -(154.0f / 600.0f), 1.0f, 0.0f,
        (26.0f / 800.0f), (154.0f / 600.0f), 1.0f, 1.0f,

        (26.0f / 800.0f), (154.0f / 600.0f), 1.0f, 1.0f,
        -(26.0f / 800.0f), (154.0f / 600.0f), 0.0f, 1.0f,
        -(26.0f / 800.0f), -(154.0f / 600.0f), 0.0f, 0.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(pipeVertices), pipeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}




void updateBirdPosition() {
    birdYAcceleration += gravity * deltaTime;
    if (birdYAcceleration > terminalVelocity) birdYAcceleration = terminalVelocity;
    birdY += birdYAcceleration * deltaTime;
}

void updatePipes() {
    for (auto& pipe : pipes) {
        pipe.first -= pipeVelocity * deltaTime;
    }

    pipes.erase(std::remove_if(pipes.begin(), pipes.end(),
        [](const std::pair<float, std::pair<float, float>>& pipe) { return pipe.first + pipeWidth < -200; }),
        pipes.end());
}


void update(GLFWwindow* window) {
    if (!gameOver) {
        updateBirdPosition();
        updatePipes();
        pipeSpawnTimer += deltaTime;

        if (pipeSpawnTimer > minPipeSpawnInterval + static_cast<float>(rand()) / RAND_MAX * (maxPipeSpawnInterval - minPipeSpawnInterval)) {
            pipeSpawnTimer = 0.0f;

            float lastGapPosition = pipes.empty() ? minPosition : pipes.back().second.second;

            std::uniform_real_distribution<float> heightDistribution(minHeight, minHeight + heightRange);
            std::uniform_real_distribution<float> positionDistribution(lastGapPosition - positionRange, lastGapPosition + positionRange);

            float upperPipeHeight = heightDistribution(generator);
            float gapPosition = positionDistribution(generator);

            pipes.emplace_back(std::make_pair(800.0f, std::make_pair(upperPipeHeight, gapPosition)));
        }

        checkCollision(window);
    }
    else {
        std::cerr << "GAME OVER ?";
    }
}

float mapValueY(float x) {
    return -0.5f + (x * 1.0f / 600.0f);
}float mapValueX(float x) {
    return -0.5f + (x * 1.0f / 800.f);
}

void drawBackground() {
    glBindVertexArray(birdVAO); // You can use any VAO since the background covers the entire window
    glUseProgram(backgroundShaderProgram);

    // Draw the entire background
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glUseProgram(0);
}

void drawBird() {
    glBindVertexArray(birdVAO);
    glUseProgram(birdShaderProgram);

    // Update the y-coordinate in the vertex shader
    GLint translationYLocation = glGetUniformLocation(birdShaderProgram, "translationY");
    glUniform1f(translationYLocation, mapValueY(birdY));

    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Restore the shader program and VAO state
    glBindVertexArray(0);
    glUseProgram(0);
}

void drawPipe(float x, float upperPipeHeight, float gapPosition) {
    glBindVertexArray(pipeVAO);
    glUseProgram(pipeShaderProgram);

    GLint translationXLocation = glGetUniformLocation(pipeShaderProgram, "translationX");
    GLint translationYLocation = glGetUniformLocation(pipeShaderProgram, "translationY");
    GLint isLowerPart = glGetUniformLocation(pipeShaderProgram, "isLowerPart");

    // Draw the upper part of the pipe
    glUniform1i(isLowerPart, 1);
    glUniform1f(translationXLocation, mapValueX(x));
    glUniform1f(translationYLocation, mapValueY((upperPipeHeight)));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    //// Draw the lower part of the pip
    glUniform1i(isLowerPart, 1);
    glUniform1f(translationXLocation, mapValueX(x));
    glUniform1f(translationYLocation, mapValueY(upperPipeHeight + 600 + pipeGap));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    //// Draw the lower part of the pip
    glUniform1i(isLowerPart, 0);
    glUniform1f(translationXLocation, mapValueX(x));
    glUniform1f(translationYLocation, mapValueY(upperPipeHeight + 300 + pipeGap));
    glDrawArrays(GL_TRIANGLES, 0, 6);





    glBindVertexArray(0);
    glUseProgram(0);
}


void checkCollision(GLFWwindow* window) {
    if (birdY - birdRadius < 0 || birdY + birdRadius > 600) {
        gameOver = true;
    }

    for (const auto& pipe : pipes) {
        if ((birdX - birdRadius > pipe.first && birdX - birdRadius < pipe.first + pipeWidth) &&
            ((birdY + birdRadius > 600 - pipe.second.first) || (birdY - birdRadius < 600 - pipe.second.first - pipeGap))) {

            std::cerr << birdX << '\n';
            std::cerr << pipe.first << '\n';


            gameOver = true;
            break;
        }
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && !gameOver) {
        if (timeSinceLastJump >= jumpTimer) {
            timeSinceLastJump = 0;
            birdYAcceleration += jumpVelocity;
        }
    }
}

GLuint loadTexture(const char* path, int& width, int& height) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int channels;
    unsigned char* image = stbi_load(path, &width, &height, &channels, 0);

    if (image) {
        GLenum format = channels == 4 ? GL_RGBA : GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
    }
    else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    return textureID;
}