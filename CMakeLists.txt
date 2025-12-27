cmake_minimum_required(VERSION 3.16)
project(VulkanCube LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ------------------------------
# Vulkan (system package)
# ------------------------------
find_package(Vulkan REQUIRED)

# ------------------------------
# GLM (header-only math library)
# ------------------------------
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
)
FetchContent_MakeAvailable(glm)

# ------------------------------
# GLFW (window and input)
# ------------------------------
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ------------------------------
# STB Image (header-only for texture loading)
# ------------------------------
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE include)

# ------------------------------
# Find glslc (from Vulkan SDK)
# ------------------------------
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin)
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found! Make sure Vulkan SDK is installed and VULKAN_SDK environment variable is set.")
endif()

# ------------------------------
# Helper function to compile shaders to SPIR-V
# ------------------------------
function(add_spv_shader TARGET SHADER_FILE OUTPUT_SPV)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/${OUTPUT_SPV}
        COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE} -o ${CMAKE_BINARY_DIR}/${OUTPUT_SPV}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE}
        COMMENT "Compiling shader ${SHADER_FILE} to SPIR-V"
    )
    target_sources(${TARGET} PRIVATE ${CMAKE_BINARY_DIR}/${OUTPUT_SPV})
endfunction()

# ------------------------------
# Executable
# ------------------------------
add_executable(vulkan_cube
    main.cpp
    vulkan/VulkanRenderer.cpp
    vulkan/VulkanInstance.cpp
    vulkan/VulkanDevice.cpp
    vulkan/VulkanSwapchain.cpp
    vulkan/VulkanRenderPass.cpp
    vulkan/VulkanGraphicsPipeline.cpp
    vulkan/VulkanShader.cpp
    vulkan/VulkanCommand.cpp
    vulkan/VulkanSync.cpp
    vulkan/VulkanSurface.cpp
    vulkan/VulkanFrame.cpp
    vulkan/Mesh.cpp
)

target_link_libraries(vulkan_cube PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    stb_image
)

target_include_directories(vulkan_cube PRIVATE
    include
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan
    ${glfw_SOURCE_DIR}/include  # ← Correct placement: after target is defined
)

# ------------------------------
# Precompile shaders to SPIR-V
# ------------------------------
add_spv_shader(vulkan_cube shaders/triangle.vert shaders/triangle.vert.spv)
add_spv_shader(vulkan_cube shaders/triangle.frag shaders/triangle.frag.spv)
add_spv_shader(vulkan_cube shaders/cube.vert shaders/cube.vert.spv)
add_spv_shader(vulkan_cube shaders/cube.frag shaders/cube.frag.spv)

# ------------------------------
# Copy texture for runtime override (modding support)
# ------------------------------
add_custom_command(
    TARGET vulkan_cube POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/container.jpg
        $<TARGET_FILE_DIR:vulkan_cube>/container.jpg
    COMMENT "Copying container.jpg to output directory"
)