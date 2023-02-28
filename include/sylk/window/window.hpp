//
// Created by August on 25-2-23.
//

#pragma once

#include <sylk/coreutils/typedefs.hpp>
#include <sylk/data/color.hpp>

class GLFWwindow;

namespace sylk {

    void window_resize_callback(GLFWwindow* window, i32 width, i32 height);

    struct WindowSettings {
        const char* title = "Sylk";
        i32 width = 1280;
        i32 height = 720;
        bool fullscreen = false;

        i32 version_major = 3;
        i32 version_minor = 3;
    };

    class Window {
        GLFWwindow* window;

    public:
        explicit Window(WindowSettings settings = {});
        ~Window();

        void clear_color(Color color) const;
        void clear() const;
        void poll_events() const;
        void render() const;

        [[nodiscard]] bool is_open() const;
    };
}
