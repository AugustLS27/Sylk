cmake_minimum_required(VERSION 3.24)
project(sylk VERSION 0.0.1)

add_compile_definitions(SYLK_VERSION="${CMAKE_PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(GIT_SHALLOW TRUE)

# spdlog
include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.11.0
)
FetchContent_MakeAvailable(spdlog)

# opengl/glfw
find_package(OpenGL REQUIRED)

include_directories(${OPENGL_INCLUDE_DIRS})

set( GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_INSTALL OFF CACHE BOOL  "GLFW lib only" )

# vulkan
find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

if(Vulkan_FOUND)
    message(STATUS "- Found Vulkan ${Vulkan_VERSION}")
    message(STATUS "- Imported includes from ${Vulkan_INCLUDE_DIR}")
    message(STATUS "- Linking Vulkan library from ${Vulkan_LIBRARY}")
else()
    message(WARNING "Could not find Vulkan.")
endif()

if(Vulkan_glslc_FOUND)
    message(STATUS "- Found GLSL SPIR-V compiler in ${Vulkan_GLSLC_EXECUTABLE}")
else()
    message(WARNING "Could not find GLSL SPIR-V compiler")
endif()


# sylk
if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    add_compile_definitions(SYLK_DEBUG)
elseif(${CMAKE_BUILD_TYPE} STREQUAL Verbose)
    add_compile_definitions(SYLK_VERBOSE)
    add_compile_definitions(SYLK_DEBUG)
else()
    add_compile_definitions(SYLK_RELEASE)
endif()

add_executable(sylk
        libs/glad/glad.c
        libs/img/stb_image.cpp

        src/main.cpp
        src/vulkan/window/vulkan_window.cpp
        src/vulkan/utils/result_handler.cpp
        src/vulkan/utils/validation_layers.cpp
        src/vulkan/window/swapchain.cpp
        src/vulkan/window/graphics_pipeline.cpp
        src/vulkan/shader/shader.cpp
        )

target_include_directories(sylk PRIVATE
        src/
        include/
        include/libs
        )

target_link_libraries(sylk PRIVATE
        spdlog::spdlog
        ${OPENGL_LIBRARIES}
        Vulkan::Vulkan
        glfw
        )
