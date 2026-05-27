class App {
public:
    inline void run() {
        this->initVulkan();
        this->renderLoop();
        this->cleanup();
    }
private:
    void initVulkan(); void renderLoop(); void cleanup();
};