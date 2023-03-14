//
// Created by August Silva on 7-3-23.
//

#ifndef SYLK_VULKAN_WINDOW_SWAPCHAIN_HPP
#define SYLK_VULKAN_WINDOW_SWAPCHAIN_HPP

#include <sylk/core/utils/short_types.hpp>

#include <sylk/vulkan/memory/buffer.hpp>
#include <sylk/vulkan/shader/vertex.hpp>
#include <sylk/vulkan/vulkan.hpp>
#include <sylk/vulkan/window/graphics_pipeline.hpp>

#include <vector>

struct GLFWwindow;

namespace sylk {

    class Swapchain {
      public:
        struct SupportDetails {
            vk::SurfaceCapabilitiesKHR        surface_capabilities;
            std::vector<vk::SurfaceFormatKHR> surface_formats;
            std::vector<vk::PresentModeKHR>   present_modes;
        };

      public:
        explicit Swapchain(const vk::Device& device);

        void create(vk::PhysicalDevice physical_device, GLFWwindow* window, vk::SurfaceKHR surface);
        void recreate();
        void destroy();
        void draw_next();

        SYLK_NODISCARD auto query_device_support_details(vk::PhysicalDevice device, vk::SurfaceKHR surface) const -> SupportDetails;
        void                set_queues(vk::Queue graphics, vk::Queue present);

      private:
        void setup_swapchain();
        void destroy_partial();
        void create_image_views();
        void create_renderpass();
        void create_command_pool();
        void create_command_buffer();
        void create_framebuffers();
        void create_synchronizers();
        void create_uniform_buffers();
        void create_descriptor_pool();
        void update_uniform_buffers();
        void create_descriptor_sets();

        template<typename T>
        void create_staged_buffer(Buffer& buffer, vk::BufferUsageFlags buffer_type, const std::vector<T>& data);
        void record_command_buffer(vk::CommandBuffer buffer, u32 image_index);

        auto select_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) const -> vk::SurfaceFormatKHR;
        auto select_present_mode(const std::vector<vk::PresentModeKHR>& available_modes) const -> vk::PresentModeKHR;
        auto select_extent_2d(vk::SurfaceCapabilitiesKHR capabilities, GLFWwindow* window) const -> vk::Extent2D;

      private:
        u32              current_frame_;
        u32              graphics_queue_family_index_;
        GraphicsPipeline graphics_pipeline_;

        GLFWwindow* window_;

        const vk::Device&  device_;
        vk::PhysicalDevice physical_device_;
        vk::SurfaceKHR     surface_;
        vk::SwapchainKHR   swapchain_;
        vk::Format         format_;
        vk::Extent2D       extent_;
        vk::RenderPass     renderpass_;

        vk::Queue graphics_queue_;
        vk::Queue presentation_queue_;

        vk::DescriptorPool descriptor_pool_;
        std::vector<vk::DescriptorSet> descriptor_sets_;

        vk::CommandPool                command_pool_;
        std::vector<vk::CommandBuffer> command_buffers_;

        std::vector<vk::Semaphore> semaphores_img_available_;
        std::vector<vk::Semaphore> semaphores_render_finished_;
        std::vector<vk::Fence>     fences_in_flight_;

        std::vector<vk::Image>       images_;
        std::vector<vk::ImageView>   image_views_;
        std::vector<vk::Framebuffer> frame_buffers_;

        Buffer                    vertex_buffer_;
        const std::vector<Vertex> vertices_ = {
            Vertex {.pos = {-0.5f, -0.5f}, .color = {1.0f, 0.0f, 0.0f}},
            Vertex { .pos = {0.5f, -0.5f}, .color = {0.0f, 1.0f, 0.0f}},
            Vertex {  .pos = {0.5f, 0.5f}, .color = {0.0f, 0.0f, 1.0f}},
            Vertex { .pos = {-0.5f, 0.5f}, .color = {0.5f, 0.5f, 0.5f}},
        };

        Buffer                 index_buffer_;
        const std::vector<u16> indices_ = {0, 1, 2, 2, 3, 0};

        std::vector<Buffer> uniform_buffers_;
    };

}  // namespace sylk

#endif  // SYLK_VULKAN_WINDOW_SWAPCHAIN_HPP
