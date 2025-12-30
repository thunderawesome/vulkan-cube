cmake_minimum_required(VERSION 3.16)
project(VulkanCube LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ------------------------------
# Dependencies
# ------------------------------
find_package(Vulkan REQUIRED)

FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.2)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE include/stb)

# ------------------------------
# Shader Compiler Discovery
# ------------------------------
# Try to find glslc first, then fallback to glslangValidator (common on macOS/Homebrew)
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin $ENV{VULKAN_SDK}/bin)
find_program(GLSLANG_VALIDATOR glslangValidator HINTS $ENV{VULKAN_SDK}/Bin $ENV{VULKAN_SDK}/bin)

if(GLSLC)
    set(SHADER_COMPILER_BIN ${GLSLC})
    message(STATUS "Using glslc for shader embedding")
elseif(GLSLANG_VALIDATOR)
    set(SHADER_COMPILER_BIN ${GLSLANG_VALIDATOR})
    message(STATUS "Using glslangValidator for shader embedding")
else()
    message(FATAL_ERROR "No shader compiler (glslc or glslangValidator) found! Required for embedding.")
endif()

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
    src/Mesh.cpp
    src/Primitive.cpp
    src/Material.cpp
    src/Renderer.cpp
    src/Scene.cpp
    src/SceneBuilder.cpp
    src/Texture.cpp
    src/VulkanUtils.cpp
    src/AssetManager.cpp
    include/stb/stb_image.cpp
)

target_link_libraries(vulkan_cube PRIVATE Vulkan::Vulkan glfw glm::glm stb_image)
target_include_directories(vulkan_cube PRIVATE include ${CMAKE_BINARY_DIR})

# ------------------------------
# Embed Shaders (Compile to SPV then Hex)
# ------------------------------
set(EMBEDDED_SHADERS_HEADER ${CMAKE_BINARY_DIR}/shaders_embedded.h)

add_custom_command(
    OUTPUT ${EMBEDDED_SHADERS_HEADER}
    COMMAND ${CMAKE_COMMAND}
        -D GLSLC_PATH=${SHADER_COMPILER_BIN}
        -D INPUT_VERT=${CMAKE_CURRENT_SOURCE_DIR}/shaders/cube.vert
        -D INPUT_FRAG=${CMAKE_CURRENT_SOURCE_DIR}/shaders/cube.frag
        -D OUTPUT_HEADER=${EMBEDDED_SHADERS_HEADER}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_vulkan_shaders.cmake
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/cube.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/cube.frag
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_vulkan_shaders.cmake
    COMMENT "Compiling and embedding Vulkan shaders"
)

add_custom_target(embed_shaders DEPENDS ${EMBEDDED_SHADERS_HEADER})
add_dependencies(vulkan_cube embed_shaders)

# ------------------------------
# Embed Texture (container.jpg)
# ------------------------------
set(EMBEDDED_TEXTURE_HEADER ${CMAKE_BINARY_DIR}/embedded_texture.h)

add_custom_command(
    OUTPUT ${EMBEDDED_TEXTURE_HEADER}
    COMMAND ${CMAKE_COMMAND}
        -D INPUT_TEXTURE=${CMAKE_CURRENT_SOURCE_DIR}/container.jpg
        -D OUTPUT_HEADER=${EMBEDDED_TEXTURE_HEADER}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_texture.cmake
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/container.jpg
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_texture.cmake
    COMMENT "Embedding texture into byte array"
)

add_custom_target(embed_texture DEPENDS ${EMBEDDED_TEXTURE_HEADER})
add_dependencies(vulkan_cube embed_texture)