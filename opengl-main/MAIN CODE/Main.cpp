#include <filesystem>
#include <queue>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include "Mesh.h"
#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Camera.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace fs = std::filesystem;

// Global variable to store G-code target positions
std::queue<glm::vec3> gcodeTargets;
glm::vec3 targetPos = glm::vec3(0.0f, 0.0f, 0.0f);

const unsigned int width = 1200;
const unsigned int height = 800;

Vertex vertices[] =
{
    Vertex{glm::vec3(-1.0f, 0.0f,  1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
    Vertex{glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 1.0f)},
    Vertex{glm::vec3(1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 1.0f)},
    Vertex{glm::vec3(1.0f, 0.0f,  1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f)}
};

GLuint indices[] =
{
    0, 1, 2,
    0, 2, 3
};

Vertex lightVertices[] =
{
    Vertex{glm::vec3(-0.1f, -0.1f,  0.1f)},
    Vertex{glm::vec3(-0.1f, -0.1f, -0.1f)},
    Vertex{glm::vec3(0.1f, -0.1f, -0.1f)},
    Vertex{glm::vec3(0.1f, -0.1f,  0.1f)},
    Vertex{glm::vec3(-0.1f,  0.1f,  0.1f)},
    Vertex{glm::vec3(-0.1f,  0.1f, -0.1f)},
    Vertex{glm::vec3(0.1f,  0.1f, -0.1f)},
    Vertex{glm::vec3(0.1f,  0.1f,  0.1f)}
};

GLuint lightIndices[] =
{
    0, 1, 2,
    0, 2, 3,
    0, 4, 7,
    0, 7, 3,
    3, 7, 6,
    3, 6, 2,
    2, 6, 5,
    2, 5, 1,
    1, 5, 4,
    1, 4, 0,
    4, 5, 6,
    4, 6, 7
};

// Function to draw coordinate lines
void drawCoordinateLines(Shader& shader, Camera& camera, float scale)
{
    // Create VAO, VBO for the lines
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    std::vector<glm::vec3> lines;
    for (float i = -scale; i <= scale; i += 1.0f)
    {
        // X-axis lines
        lines.push_back(glm::vec3(i, 0.0f, -scale));
        lines.push_back(glm::vec3(i, 0.0f, scale));

        // Z-axis lines
        lines.push_back(glm::vec3(-scale, 0.0f, i));
        lines.push_back(glm::vec3(scale, 0.0f, i));
    }

    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3), &lines[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    shader.Activate();
    camera.Matrix(shader, "camMatrix");

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, lines.size());

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

// Function to execute G-code commands
void executeGcode(const char* gcode, std::queue<glm::vec3>& targets, float minX, float maxX, float minY, float maxY, float minZ, float maxZ)
{
    std::istringstream stream(gcode);
    std::string line;

    while (std::getline(stream, line)) {
        std::istringstream linestream(line);
        std::string command;
        linestream >> command;

        if (command == "G0" || command == "G1") {
            float x = targetPos.x;
            float y = targetPos.y;
            float z = targetPos.z;
            char axis;
            float value;

            while (linestream >> axis >> value) {
                if (axis == 'X') {
                    x = value;
                }
                else if (axis == 'Y') {
                    y = value;
                }
                else if (axis == 'Z') {
                    z = value;
                }
            }

            // Clamp values to within the allowed range
            x = std::max(minX, std::min(maxX, x));
            y = std::max(minY, std::min(maxY, y));
            z = std::max(minZ, std::min(maxZ, z));

            // Push the new target position to the queue
            targets.push(glm::vec3(x, y, z));
        }
        // Handle other G-code commands as needed
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Buffer to hold the G-code input text
    char gcodeInputText[1024] = "";

    GLFWwindow* window = glfwCreateWindow(width, height, "OpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0, 0, width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::string parentDir = (fs::current_path().fs::path::parent_path()).string();
    std::string texPath = "/Resources/YoutubeOpenGL 10 - Specular Maps/";

    Texture textures[] =
    {
        Texture((parentDir + texPath + "planks.png").c_str(), "diffuse", 0, GL_RGBA, GL_UNSIGNED_BYTE),
        Texture((parentDir + texPath + "planksSpec.png").c_str(), "specular", 1, GL_RED, GL_UNSIGNED_BYTE)
    };

    Shader shaderProgram("default.vert", "default.frag");
    std::vector<Vertex> verts(vertices, vertices + sizeof(vertices) / sizeof(Vertex));
    std::vector<GLuint> ind(indices, indices + sizeof(indices) / sizeof(GLuint));
    std::vector<Texture> tex(textures, textures + sizeof(textures) / sizeof(Texture));
    Mesh floor(verts, ind, tex);

    Shader lightShader("light.vert", "light.frag");
    std::vector<Vertex> lightVerts(lightVertices, lightVertices + sizeof(lightVertices) / sizeof(Vertex));
    std::vector<GLuint> lightInd(lightIndices, lightIndices + sizeof(lightIndices) / sizeof(GLuint));
    Mesh light(lightVerts, lightInd, tex);

    glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 0.0f);
    targetPos = lightPos;
    glm::mat4 lightModel = glm::mat4(1.0f);
    lightModel = glm::translate(lightModel, lightPos);

    glm::vec3 objectPos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::mat4 objectModel = glm::mat4(1.0f);
    objectModel = glm::translate(objectModel, objectPos);

    float floorScale = 5.0f;
    objectModel = glm::scale(objectModel, glm::vec3(floorScale, 1.0f, floorScale));

    lightShader.Activate();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lightModel));
    glUniform4f(glGetUniformLocation(lightShader.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
    shaderProgram.Activate();
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "model"), 1, GL_FALSE, glm::value_ptr(objectModel));
    glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
    glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);

    glEnable(GL_DEPTH_TEST);

    Camera camera(width, height, glm::vec3(0.0f, 0.0f, 5.0f));

    glm::vec3 lightVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
    float minX = -floorScale;
    float maxX = floorScale;
    float minY = 0.0f;
    float maxY = 2.0f;
    float minZ = -floorScale;
    float maxZ = floorScale;

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Variable to hold the selected radio button index
    static int selected = 0;

    // Variable to store the control mode
    static bool controlModeArrows = true;

    // Interpolation speed
    const float moveSpeed = 0.01f;  // Speed of light movement

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Check if the Alt key is pressed
        bool altPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

        // Only handle camera inputs if Alt is pressed
        if (altPressed)
        {
            camera.Inputs(window);
        }
        camera.updateMatrix(45.0f, 0.1f, 100.0f);

        // Control light position via G-code
        if (!controlModeArrows)
        {
            if (!gcodeTargets.empty())
            {
                glm::vec3 target = gcodeTargets.front();
                glm::vec3 direction = target - lightPos;
                float distance = glm::length(direction);

                if (distance > moveSpeed)
                {
                    direction = glm::normalize(direction);
                    lightPos += direction * moveSpeed;
                }
                else
                {
                    lightPos = target;
                    gcodeTargets.pop();
                }

                targetPos = lightPos;
            }
        }
        else
        {
            // Arrow key control
            lightVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && lightPos.z - 0.01f >= minZ) {
                lightVelocity.z = -0.01f;
            }
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && lightPos.z + 0.01f <= maxZ) {
                lightVelocity.z = 0.01f;
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && lightPos.x - 0.01f >= minX) {
                lightVelocity.x = -0.01f;
            }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && lightPos.x + 0.01f <= maxX) {
                lightVelocity.x = 0.01f;
            }
            if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS && lightPos.y + 0.01f <= maxY) {
                lightVelocity.y = 0.01f;
            }
            if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS && lightPos.y - 0.01f >= minY) {
                lightVelocity.y = -0.01f;
            }

            lightPos += lightVelocity;

            // Reset target position after arrow key control
            targetPos = lightPos;
        }

        lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lightPos);
        lightShader.Activate();
        glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lightModel));

        shaderProgram.Activate();
        glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);

        floor.Draw(shaderProgram, camera);

        // Draw coordinate lines
        drawCoordinateLines(shaderProgram, camera, floorScale);

        light.Draw(lightShader, camera);

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create a simple window with radio buttons and an input box
        ImGui::Begin("Control Panel");
        ImGui::Text("Choose control mode:");
        ImGui::RadioButton("Arrow Keys", &selected, 0);
        ImGui::RadioButton("G-code", &selected, 1);

        // Update control mode based on ImGui selection
        controlModeArrows = (selected == 0);

        if (!controlModeArrows) {
            ImGui::InputTextMultiline("G-code Input", gcodeInputText, IM_ARRAYSIZE(gcodeInputText), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16));
            if (ImGui::Button("Execute")) {
                executeGcode(gcodeInputText, gcodeTargets, minX, maxX, minY, maxY, minZ, maxZ);
            }
        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    shaderProgram.Delete();
    lightShader.Delete();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
