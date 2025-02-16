#pragma once
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>

#include "../utils/utils.hpp"
#include "./Shader.cpp"
#include <experimental/filesystem>

using GameWindow = int;

static const struct
{
    float x, y;
    float r, g, b;
}
vertices[6] =
{
  {-0.6f, -0.4f, 1.f, 0.f, 0.f},
  {0.6f, -0.4f, 0.f, 1.f, 0.f},
  {0.f, 0.6f, 0.f, 0.f, 1.f},
  {0.6f, -0.4f, 0.f, 0.f, 0.f},
  {-0.6f, 0.4f, 0.f, 1.f, 0.f},
  {0.f, -0.6f, 0.f, 0.f, 1.f},
};

using namespace Utils;
class Engine {
    std::vector<GLFWwindow*> GameWindows;
    public:
    /*
    Constructor with a required error_callback function pointer parameter
    */
    Engine(void(*error_callback)(int error_code, const char* description)) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
            exit(EXIT_FAILURE);

#ifdef DEBUG
        Println<const char*>("OpenGL loaded 🤩");
#endif
    }


    /*
    Run the OpenGL loop on the GLFWwindow specified by its index in the GameWindows Vector
    */
    void Run(void(*process_input)(GLFWwindow*), GameWindow win_idx) {
        GLint mvp_location, vpos_location, vcol_location;

        // TODO: WIP
        unsigned int VAO, VBO;// Vertex Array Object, Vertex Buffer Object
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);

        Shader triangleShader(FileFromSourceDir("/shaders/triangle.vs").c_str(), FileFromSourceDir("/shaders/triangle.fs").c_str());

        mvp_location = glGetUniformLocation(triangleShader.ID, "MVP");
        vpos_location = glGetAttribLocation(triangleShader.ID, "vPos");
        vcol_location = glGetAttribLocation(triangleShader.ID, "vCol");

        // Bind buffer and set its data
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Position attribute
        glEnableVertexAttribArray(vpos_location);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)0);

        // Color attribute
        glEnableVertexAttribArray(vcol_location);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)(2 * sizeof(vertices[0])));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        GLFWwindow* window = GameWindows.at(win_idx);

        while (!glfwWindowShouldClose(window))
        {
            process_input(window);
            glClear(GL_COLOR_BUFFER_BIT);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            triangleShader.use();
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    /*
    Creates a GLFWwindow and stores it in the GameWindows vector
    */
    int CreateWindow(int width = 1280, int height = 720, const char* title = "Minecraft 2 (legit dev build)", GLFWmonitor* monitor = NULL, GLFWwindow* share = NULL) {
        GLFWwindow* window = glfwCreateWindow(width, height, title, monitor, share);
        if (!window) {
#ifdef DEBUG
            Println<const char*>("Problem window :(");
#endif
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        GameWindows.push_back(window);
        gladLoadGL();
        glfwSwapInterval(1);
        return GameWindows.size() - 1;
    }

    /*
    Make a window current context by it's index in the GameWindows vectors
    */
    void MakeContext(GameWindow win_idx) {
        glfwMakeContextCurrent(GameWindows.at(win_idx));
    }

    /*
    Destroy all windows in the GameWindows vector and terminate OpenGL
    */
    void Kill() {
        for (const auto window : GameWindows) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
#ifdef DEBUG
        Println<const char*>("\nExiting now...\n");
#endif
        exit(EXIT_SUCCESS);
    }
};