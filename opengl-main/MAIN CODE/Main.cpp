#include <filesystem>
namespace fs = std::filesystem;

#include "Mesh.h"
#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Camera.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

const unsigned int width = 800;
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

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    // Buffer to hold the input text
    char inputText[128] = "";

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

        lightPos += lightVelocity;

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
        ImGui::Begin("Hello, world!");
        ImGui::Text("This is a simple text.");
        ImGui::RadioButton("Option 1", &selected, 0);
        ImGui::RadioButton("Option 2", &selected, 1);
        ImGui::InputText("Input Text", inputText, IM_ARRAYSIZE(inputText));
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
