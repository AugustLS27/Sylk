cmake_minimum_required(VERSION 3.24)
project(sylk VERSION 0.0.3)

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
add_executable(sylk
        libs/glad/glad.c
        libs/img/stb_image.cpp

        src/main.cpp

        src/vulkan/utils/result_handler.cpp
        src/vulkan/utils/validation_layers.cpp
        src/vulkan/utils/queue_family_indices.cpp

        src/vulkan/window/vulkan_window.cpp
        src/vulkan/window/swapchain.cpp
        src/vulkan/window/graphics_pipeline.cpp

        src/vulkan/shader/shader.cpp
        src/vulkan/shader/vertex.cpp
        )

target_include_directories(sylk PRIVATE
        include/
        include/libs
        )

target_link_libraries(sylk PRIVATE
        spdlog::spdlog
        ${OPENGL_LIBRARIES}
        Vulkan::Vulkan
        glfw
        )

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    target_compile_definitions(sylk PRIVATE SYLK_DEBUG)
elseif(${CMAKE_BUILD_TYPE} STREQUAL Verbose)
    target_compile_definitions(sylk PRIVATE SYLK_VERBOSE)
    target_compile_definitions(sylk PRIVATE SYLK_DEBUG)
else()
    target_compile_definitions(sylk PRIVATE SYLK_RELEASE)
endif()

target_compile_definitions(sylk PRIVATE SYLK_NODISCARD=[[nodiscard]])
target_compile_definitions(sylk PRIVATE SYLK_VERSION_STR="${CMAKE_PROJECT_VERSION}")
target_compile_definitions(sylk PRIVATE SYLK_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR})
target_compile_definitions(sylk PRIVATE SYLK_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR})
target_compile_definitions(sylk PRIVATE SYLK_VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH})

