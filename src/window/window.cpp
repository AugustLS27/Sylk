//
// Created by August on 25-2-23.
//

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <sylk/window/window.hpp>
#include <sylk/coreutils/all.hpp>

namespace sylk {

    void window_resize_callback(GLFWwindow* window, i32 width, i32 height) {
        glViewport(0, 0, width, height);
    }

    /// TODO: look into making window class static, as there can only be one window anyway.
    /// then again, that'd be an unnecessary restriction.
    Window::Window(const WindowSettings settings)
        : window(nullptr) {
        if (!glfwInit()) {
            log(CRITICAL, "GLFW initialization failed");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, settings.version_major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, settings.version_minor);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWmonitor* monitor = nullptr;

        if (settings.fullscreen) {
            monitor = glfwGetPrimaryMonitor();
        }

        window = glfwCreateWindow(settings.width, settings.height, settings.title, monitor, nullptr);

        if (!window) {
            glfwTerminate();
            log(CRITICAL, "Failed to create window");
        }

        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            log(CRITICAL, "Failed to initialize GLAD");
        }

        glViewport(0, 0, settings.width, settings.height);
        glfwSetFramebufferSizeCallback(window, window_resize_callback);

        clear_color({0.0f, 0.0f, 0.0f, 1.0f});
    }

    Window::~Window() {
        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }

    bool Window::is_open() const {
        return !glfwWindowShouldClose(window);
    }

    void Window::clear() const {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void Window::clear_color(Color color) const {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void Window::poll_events() const {
        glfwPollEvents();
    }

    void Window::render() const {
        glfwSwapBuffers(window);
    }
}