#include <GLFW/glfw3.h>
#include<vulkan/vulkan_raii.hpp>

class VulkanContext {
public:
    inline void init() {
        this->initInstance();
    }
private:
    vk::raii::Context vkContext;
    vk::raii::Instance vkInstance{NULL};

    void initInstance();

};

class App {
public:
    inline void run() {
        this->initWindow();
        this->initVulkan();
        this->mainLoop();
        this->cleanup();
    }
private:
    const int WINDOW_W = 800;
    const int WINDOW_H = 600;

    GLFWwindow* window;
    VulkanContext vulkanContext;

    void initWindow();
    void initVulkan(); 
    
    void mainLoop();
    void processWindowInput(); 
    
    void cleanup();
};