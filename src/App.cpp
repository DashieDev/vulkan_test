#include "App.hpp"
#include <GLFW/glfw3.h>

void App::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    this->window = glfwCreateWindow(WINDOW_W, WINDOW_H, "Vulkan Test", NULL, NULL);
}

void App::initVulkan() {

}

void App::mainLoop() {
    while (!glfwWindowShouldClose(this->window)) {
        processWindowInput();
        glfwPollEvents();
    }
}

void App::processWindowInput() {
    if(glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(this->window, true);
}

void App::cleanup() {
    glfwDestroyWindow(window);

    glfwTerminate();
}