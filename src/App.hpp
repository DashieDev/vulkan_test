#include <GLFW/glfw3.h>
#include<vulkan/vulkan_raii.hpp>

class VulkanContext {
public:
    inline void init() {
        this->initInstance();
        this->initDebugMsgr();
        this->initSurface();
        this->pickPhysicalDevice();
        this->createLogicalDevice();
    };
private:
    vk::raii::Context vkContext;
    vk::raii::Instance vkInstance{nullptr};
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    vk::PhysicalDeviceFeatures deviceFeatures;
    vk::raii::Queue queue = nullptr;

    void initInstance();
    void initDebugMsgr();
    void initSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
};

class App {
public:
    static App& get();

    App();
    ~App();

    inline void run() {
        this->initWindow();
        this->initVulkan();
        this->mainLoop();
        this->cleanup();
    }

    inline GLFWwindow* getWindow() { return this->window; }
private:
    static App* SHARED_INSTANCE;

    static const int WINDOW_W = 800;
    static const int WINDOW_H = 600;

    GLFWwindow* window;
    VulkanContext vulkanContext;

    void initWindow();
    void initVulkan(); 
    
    void mainLoop();
    void processWindowInput(); 
    
    void cleanup();
};