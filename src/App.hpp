#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

class VulkanContext {
public:
    inline void init() {
        this->initInstance();
        this->initDebugMsgr();
        this->initSurface();
        this->pickPhysicalDevice();
        this->createLogicalDevice();
        this->createSwapChain();
        this->createImageViews();
        this->setupRenderPipeline();
        this->createCommandPool();
        this->createCommandBuffer();
        this->createAsyncHelper();
    };

    void renderFrame();
private:
    vk::raii::Context vkContext;
    vk::raii::Instance vkInstance{nullptr};
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::PhysicalDeviceFeatures deviceFeatures;
    vk::raii::Device device = nullptr;
    vk::raii::Queue queue = nullptr;
    uint32_t queueFamilyIndex; 

    vk::raii::SwapchainKHR swapChain = nullptr;
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::raii::ImageView> swapChainImageViews;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline renderPipeline = nullptr;

    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::CommandBuffer commandBuffer = nullptr;

    vk::raii::Semaphore whenImageAccquired = nullptr;
    std::vector<vk::raii::Semaphore> whenRenderedToImage;

    vk::raii::Fence whenRenderedToFrameJoin = nullptr;

    void initInstance();
    void initDebugMsgr();
    void initSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void setupRenderPipeline();
    void createCommandPool();
    void createCommandBuffer();
    void createAsyncHelper();

    void recordCommandBuffer(uint32_t imageIndex);
    inline void transitionImageLayout(
        uint32_t imageIndx,
        vk::ImageLayout fromLayout, vk::ImageLayout toLayout,
        vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask
    ) {
        
        auto barrier = vk::ImageMemoryBarrier2()
            .setSrcStageMask(srcStageMask).setDstStageMask(dstStageMask)
            .setSrcAccessMask(srcAccessMask).setDstAccessMask(dstAccessMask)
            .setOldLayout(fromLayout).setNewLayout(toLayout)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(this->swapChainImages[imageIndx])
            .setSubresourceRange(
                vk::ImageSubresourceRange()
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseMipLevel(0)
                    .setLevelCount(1)
                    .setBaseArrayLayer(0)
                    .setLayerCount(1)
            );
        auto dependency_args = vk::DependencyInfo()
            .setDependencyFlags({})
            .setImageMemoryBarriers(barrier);
        this->commandBuffer.pipelineBarrier2(dependency_args);
    }
};

class App {
public:
    static App& get();

    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    App(App&&) = delete;
    App& operator=(App&&) = delete;

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