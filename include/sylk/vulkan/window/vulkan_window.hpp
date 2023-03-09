//
// Created by August on 25-2-23.
//

#pragma once

#include <sylk/core/utils/rust_style_types.hpp>
#include <sylk/vulkan/utils/validation_layers.hpp>
#include <sylk/vulkan/window/swapchain.hpp>
#include <sylk/vulkan/window/graphics_pipeline.hpp>

#include <vector>
#include <span>
#include <optional>

#include <vulkan/vulkan.hpp>

struct GLFWwindow;

// The functions in this class use auto return type as a reminder to myself
// To check whether that is what I'd like to go with in the future

namespace sylk {
    class VulkanWindow {
        struct Settings {
            Settings();

            const char* title = "Sylk";
            i32 width = 1280;
            i32 height = 720;
            bool fullscreen = false;
        };

        struct QueueFamilyIndices {
            std::optional<u32> graphics;
            std::optional<u32> presentation;

            [[nodiscard]] bool has_required() const {
                return graphics.has_value() && presentation.has_value();
            }
        };

    public:
        explicit VulkanWindow(Settings settings = {});
        ~VulkanWindow();

        void poll_events() const;
        void render() const;

        auto is_open() const -> bool;

    private:
        void init_glfw();
        void create_instance();
        void select_physical_device();
        void create_logical_device();
        void create_surface();
        void create_swapchain();
        void create_renderpass();

        auto fetch_required_extensions(bool force_update = false) -> std::span<const char*>;
        auto required_extensions_available() -> bool;
        auto device_supports_required_extensions(vk::PhysicalDevice device) const -> bool;
        auto find_queue_families(vk::PhysicalDevice device) const -> QueueFamilyIndices;
        auto device_is_suitable(vk::PhysicalDevice device) const -> bool;

    private:
        GLFWwindow* window_;

        // sylk
        Settings settings_;
        ValidationLayers validation_layers_;
        Swapchain swapchain_;
        GraphicsPipeline graphics_pipeline_;

        // stl
        std::vector<const char*> required_extensions_;
        std::vector<const char*> available_extensions_;
        static constexpr std::array required_device_extensions_ {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        // vulkan
        vk::Queue graphics_queue_;
        vk::Queue presentation_queue_;
        vk::Instance instance_;
        vk::Device device_;
        vk::PhysicalDevice physical_device_;
        vk::SurfaceKHR surface_;
        vk::RenderPass renderpass_;

    };
}
