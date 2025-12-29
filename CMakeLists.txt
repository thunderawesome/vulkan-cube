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
# Find shader compiler: prefer glslc, fallback to glslangValidator
# ------------------------------
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin $ENV{VULKAN_SDK}/bin)
find_program(GLSLANG_VALIDATOR glslangValidator)

if(GLSLC)
    set(SHADER_COMPILER ${GLSLC})
    set(SHADER_COMPILER_FLAGS "")
    message(STATUS "Using glslc for shader compilation")
elseif(GLSLANG_VALIDATOR)
    set(SHADER_COMPILER ${GLSLANG_VALIDATOR})
    set(SHADER_COMPILER_FLAGS -V)  # Enable Vulkan semantics and SPIR-V output
    message(STATUS "Using glslangValidator as fallback for shader compilation")
else()
    message(FATAL_ERROR "No GLSL-to-SPIR-V compiler found! Install either glslc (Vulkan SDK) or glslangValidator.")
endif()

# ------------------------------
# Helper function to compile shaders to SPIR-V
# ------------------------------
function(add_spv_shader TARGET SHADER_FILE OUTPUT_SPV)
    # Determine shader stage from file extension for glslangValidator
    get_filename_component(SHADER_EXT ${SHADER_FILE} EXT)

    set(STAGE_FLAG "")
    if(SHADER_COMPILER STREQUAL ${GLSLANG_VALIDATOR})
        if(SHADER_EXT STREQUAL ".vert")
            set(STAGE_FLAG -S vert)
        elseif(SHADER_EXT STREQUAL ".frag")
            set(STAGE_FLAG -S frag)
        elseif(SHADER_EXT STREQUAL ".comp")
            set(STAGE_FLAG -S comp)
        elseif(SHADER_EXT STREQUAL ".geom")
            set(STAGE_FLAG -S geom)
        elseif(SHADER_EXT STREQUAL ".tesc")
            set(STAGE_FLAG -S tesc)
        elseif(SHADER_EXT STREQUAL ".tese")
            set(STAGE_FLAG -S tese)
        # Add more stages if needed
        endif()
    endif()

    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/${OUTPUT_SPV}
        COMMAND ${SHADER_COMPILER} ${SHADER_COMPILER_FLAGS} ${STAGE_FLAG}
                ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE}
                -o ${CMAKE_BINARY_DIR}/${OUTPUT_SPV}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE}
        COMMENT "Compiling shader ${SHADER_FILE} to SPIR-V"
    )
    target_sources(${TARGET} PRIVATE ${CMAKE_BINARY_DIR}/${OUTPUT_SPV})
endfunction()

# Ensure the shaders output directory exists in the build tree
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

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
    ${glfw_SOURCE_DIR}/include
)

# ------------------------------
# Precompile shaders to SPIR-V
# ------------------------------
add_spv_shader(vulkan_cube shaders/triangle.vert shaders/triangle.vert.spv)
add_spv_shader(vulkan_cube shaders/triangle.frag shaders/triangle.frag.spv)
add_spv_shader(vulkan_cube shaders/cube.vert    shaders/cube.vert.spv)
add_spv_shader(vulkan_cube shaders/cube.frag    shaders/cube.frag.spv)

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