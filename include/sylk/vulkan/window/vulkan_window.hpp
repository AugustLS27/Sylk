//
// Created by August on 25-2-23.
//

#pragma once

#include <sylk/core/utils/rust_style_types.hpp>
#include <sylk/vulkan/utils/validation_layers.hpp>

#include <vector>
#include <span>
#include <optional>

#include <vulkan/vulkan.hpp>

struct GLFWwindow;

namespace sylk {
    class VulkanWindow {
        struct Settings {
            Settings();

            const char* title = "Sylk";
            i32 width = 1280;
            i32 height = 720;
            bool fullscreen = false;
        } settings_;

        struct QueueFamilyIndices {
            std::optional<u32> graphics;
            std::optional<u32> presentation;

            [[nodiscard]] bool has_required() const {
                return graphics.has_value() && presentation.has_value();
            }
        };

        vk::Queue graphics_queue_;
        vk::Queue presentation_queue_;

        ValidationLayers validation_layers_;

        GLFWwindow* window_;

        std::vector<const char*> required_extensions_;
        std::vector<const char*> available_extensions_;

        static constexpr std::array required_device_extensions_ {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        vk::Instance instance_;
        vk::Device device_;
        vk::PhysicalDevice physical_device_;
        vk::SurfaceKHR surface_;

    public:
        explicit VulkanWindow(Settings settings = {});
        ~VulkanWindow();

        void poll_events() const;
        void render() const;

        [[nodiscard]] bool is_open() const;
        void init();

    private:
        void create_instance();
        void select_physical_device();
        void create_logical_device();
        void create_surface();

        std::span<const char*> fetch_required_extensions(bool force_update = false);
        bool required_extensions_available(); // NOLINT(modernize-use-nodiscard)
        [[nodiscard]] bool device_is_suitable(const vk::PhysicalDevice& device,
                                              vk::PhysicalDeviceType required_device_type = vk::PhysicalDeviceType::eDiscreteGpu) const;
        [[nodiscard]] bool device_supports_required_extensions(const vk::PhysicalDevice& device) const;

        QueueFamilyIndices find_queue_families(const vk::PhysicalDevice& device) const;

    };
}
