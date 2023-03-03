cmake_minimum_required(VERSION 3.24)
project(sylk VERSION 0.0.1)

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


# sylk
if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    add_compile_definitions(SYLK_DEBUG)
else()
    add_compile_definitions(SYLK_RELEASE)
endif()

add_executable(sylk
        libs/glad/glad.c
        libs/img/stb_image.cpp

        src/main.cpp
        src/window/window.cpp
        src/shader/shader.cpp
        src/shader/program.cpp

        )

target_include_directories(sylk PRIVATE
        "src/"
        "include/"
        )

target_link_libraries(sylk PRIVATE
        spdlog::spdlog
        ${OPENGL_LIBRARIES}
        glfw
        )
