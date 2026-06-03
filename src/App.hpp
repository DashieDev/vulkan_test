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
    vk::raii::Queue graphicsQueue = nullptr;

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
private:
    static App* SHARED_INSTANCE;

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