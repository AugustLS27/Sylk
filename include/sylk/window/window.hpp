//
// Created by August on 25-2-23.
//

#pragma once

#include <vulkan/vulkan.hpp>

#include <sylk/coreutils/typedefs.hpp>
#include <sylk/data/color.hpp>

#include <vector>
#include <span>

class GLFWwindow;

namespace sylk {

    class Window {
        struct Settings {
            Settings();

            const char* title = "Sylk";
            i32 width = 1280;
            i32 height = 720;
            bool fullscreen = false;
        };

        GLFWwindow* window_;
        Settings settings_;


        std::vector<const char*> required_extensions_;

        vk::Instance vk_instance_;
        vk::Device vk_device_;
        vk::PhysicalDevice vk_phys_device_;

    public:
        explicit Window(Settings settings = {});
        ~Window();

        void poll_events() const;
        void render() const;

        void create_vk_instance();

        std::span<const char*> fetch_vk_required_extensions(bool force_update = false);

        [[nodiscard]] bool is_open() const;
    };
}
