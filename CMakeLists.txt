cmake_minimum_required(VERSION 3.20)
project(VulkanTest1 VERSION 1.0.0 LANGUAGES C CXX)

add_executable(VulkanTest1 
    "src/main.cpp"
    "src/App.hpp"
    "src/App.cpp"
    "src/utils.hpp"
    "src/utils.cpp"
)

find_package(glfw3 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

target_link_libraries(VulkanTest1 PRIVATE 
    glfw  
    ${Stb_INCLUDE_DIR}
    glm::glm
    Vulkan::Vulkan 
    GPUOpen::VulkanMemoryAllocator
)

# Use C++20
target_compile_features(VulkanTest1 PRIVATE cxx_std_20)

target_compile_definitions(VulkanTest1 PRIVATE 
    GLFW_INCLUDE_VULKAN
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

add_custom_command(
    TARGET VulkanTest1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/assets"
        "$<TARGET_FILE_DIR:VulkanTest1>/assets"
    COMMENT "Copying assets to the build directory..."
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)