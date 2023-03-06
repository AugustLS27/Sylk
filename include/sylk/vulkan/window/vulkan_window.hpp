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
            std::optional<i32> graphics;

            [[nodiscard]] bool has_all() const {
                return graphics.has_value();
            }
        };

        ValidationLayers validation_layers_;

        GLFWwindow* window_;

        std::vector<const char*> vk_required_extensions_;
        std::vector<const char*> vk_available_extensions_;

        vk::Instance vk_instance_;
        vk::Device vk_device_;
        vk::PhysicalDevice vk_physical_device_;

    public:
        explicit VulkanWindow(Settings settings = {});
        ~VulkanWindow();

        void poll_events() const;
        void render() const;

        [[nodiscard]] bool is_open() const;

    private:
        void create_vk_instance();
        void select_vk_physical_device();

        std::span<const char*> fetch_vk_required_extensions(bool force_update = false);
        bool check_vk_required_extensions_available(); // NOLINT(modernize-use-nodiscard)
        [[nodiscard]] bool is_device_suitable(const vk::PhysicalDevice& device,
                                              vk::PhysicalDeviceType required_device_type = vk::PhysicalDeviceType::eDiscreteGpu) const;

        QueueFamilyIndices find_queue_families(const vk::PhysicalDevice& device) const;

    };
}
