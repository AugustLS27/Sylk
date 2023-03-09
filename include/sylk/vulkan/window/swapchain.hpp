//
// Created by August Silva on 7-3-23.
//

#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>
#include <sylk/core/utils/rust_style_types.hpp>

struct GLFWwindow;

namespace sylk {

    class Swapchain {
    public:
        struct SupportDetails {
            vk::SurfaceCapabilitiesKHR surface_capabilities;
            std::vector<vk::SurfaceFormatKHR> surface_formats;
            std::vector<vk::PresentModeKHR> present_modes;
        };

        struct CreateParams {
            GLFWwindow* window;
            Swapchain::SupportDetails support_details;
            vk::Device device;
            vk::SurfaceKHR surface;
            vk::SharingMode sharing_mode;
            std::vector<u32>& queue_family_indices;
        };

    public:
        void create(CreateParams params);
        void destroy();

        const std::vector<vk::Image>& retrieve_images() const;
        vk::Format get_format() const;
        vk::Extent2D get_extent() const;

        SupportDetails query_device_support_details(vk::PhysicalDevice device, vk::SurfaceKHR surface) const;

    private:
        void create_image_views();

        vk::SurfaceFormatKHR select_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) const;
        vk::PresentModeKHR select_present_mode(const std::vector<vk::PresentModeKHR>& available_modes) const;
        vk::Extent2D select_extent_2d(const vk::SurfaceCapabilitiesKHR capabilities, GLFWwindow* window) const;

    private:
        vk::SwapchainKHR swapchain_;
        vk::Format format_;
        vk::Extent2D extent_;
        std::vector<vk::Image> images_;
        std::vector<vk::ImageView> image_views_;
        vk::Device device_;
    };

}
