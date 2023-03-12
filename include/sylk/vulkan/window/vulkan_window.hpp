//
// Created by August on 25-2-23.
//

#ifndef SYLK_VULKAN_WINDOW_VULKANWINDOW_HPP
#define SYLK_VULKAN_WINDOW_VULKANWINDOW_HPP

#include <sylk/core/utils/short_types.hpp>
#include <sylk/vulkan/utils/validation_layers.hpp>
#include <sylk/vulkan/window/swapchain.hpp>
#include <sylk/vulkan/window/graphics_pipeline.hpp>
#include <sylk/vulkan/vulkan.hpp>

#include <vector>
#include <span>
#include <optional>

struct GLFWwindow;

namespace sylk {
    class VulkanWindow {
        struct Settings {
            Settings();

            const char* title = "Sylk";
            i32 width = 1280;
            i32 height = 720;
            bool fullscreen = false;
        };

    public:
        explicit VulkanWindow(Settings settings = {});
        ~VulkanWindow();

        void poll_events() const;
        void render();

        auto is_open() const -> bool;

    private:
        void create_window();
        void create_instance();
        void select_physical_device();
        void create_logical_device();
        void create_surface();

        auto fetch_required_extensions(bool force_update = false) -> std::span<const char*>;
        auto required_extensions_available() -> bool;
        auto device_supports_required_extensions(vk::PhysicalDevice device) const -> bool;
        auto device_is_suitable(vk::PhysicalDevice device) const -> bool;

    private:
        GLFWwindow* window_;

        Settings settings_;
        ValidationLayers validation_layers_;
        Swapchain swapchain_;

        std::vector<const char*> required_extensions_;
        std::vector<const char*> available_extensions_;
        static constexpr std::array required_device_extensions_ {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        vk::Instance instance_;
        vk::Device device_;
        vk::PhysicalDevice physical_device_;
        vk::SurfaceKHR surface_;
    };
}

#endif // SYLK_VULKAN_WINDOW_VULKANWINDOW_HPP