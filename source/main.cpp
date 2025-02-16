#include <iostream>
#include <string>
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "utils/utils.hpp"
#include "classes/Engine.cpp"

static void error_callback(int error, const char *description);

void processInput(GLFWwindow *window);

int main(int argc, char* argv[]) {
  using namespace Utils;
  Engine engine = Engine(&error_callback);
  GameWindow mainWindowIndex = engine.CreateWindow();
  engine.Run(&processInput, mainWindowIndex);
  return 0;
}

static void error_callback(int error, const char *description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}
