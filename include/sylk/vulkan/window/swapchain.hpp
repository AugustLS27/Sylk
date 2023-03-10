//
// Created by August Silva on 7-3-23.
//

#pragma once

#include <sylk/core/utils/rust_style_types.hpp>
#include <sylk/vulkan/window/graphics_pipeline.hpp>

#include <vulkan/vulkan.hpp>

#include <vector>

struct GLFWwindow;

namespace sylk {

    class Swapchain {
    public:
        struct SupportDetails {
            vk::SurfaceCapabilitiesKHR surface_capabilities;
            std::vector<vk::SurfaceFormatKHR> surface_formats;
            std::vector<vk::PresentModeKHR> present_modes;
        };

    public:
        explicit Swapchain(const vk::Device& device);
        void create(vk::PhysicalDevice physical_device, GLFWwindow* window, vk::SurfaceKHR surface);
        void destroy();

        void draw_next();

        SupportDetails query_device_support_details(vk::PhysicalDevice device, vk::SurfaceKHR surface) const;
        void set_queues(vk::Queue graphics, vk::Queue present);

    private:
        void create_image_views();
        void create_renderpass();
        void create_command_pool();
        void create_command_buffer();
        void create_framebuffers();
        void create_synchronizers();

        void record_command_buffer(vk::CommandBuffer buffer, u32 image_index);


        vk::SurfaceFormatKHR select_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) const;
        vk::PresentModeKHR select_present_mode(const std::vector<vk::PresentModeKHR>& available_modes) const;
        vk::Extent2D select_extent_2d(const vk::SurfaceCapabilitiesKHR capabilities, GLFWwindow* window) const;

    private:
        u32 graphics_queue_family_index_;
        GraphicsPipeline graphics_pipeline_;

        const vk::Device& device_;
        vk::SwapchainKHR swapchain_;
        vk::Format format_;
        vk::Extent2D extent_;
        vk::RenderPass renderpass_;

        vk::Queue graphics_queue_;
        vk::Queue presentation_queue_;

        std::vector<vk::CommandBuffer> command_buffers_;
        vk::CommandPool command_pool_;

        vk::Semaphore sema_img_available_;
        vk::Semaphore sema_render_finished_;
        vk::Fence fence_in_flight_;

        std::vector<vk::Image> images_;
        std::vector<vk::ImageView> image_views_;
        std::vector<vk::Framebuffer> frame_buffers_;
    };

}
