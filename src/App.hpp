#include <GLFW/glfw3.h>

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

    void initWindow();
    void initVulkan(); 
    
    void mainLoop();
    void processWindowInput(); 
    
    void cleanup();
};