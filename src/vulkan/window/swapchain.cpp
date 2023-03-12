//
// Created by August Silva on 7-3-23.
//

#include <sylk/vulkan/window/swapchain.hpp>
#include <sylk/vulkan/utils/queue_family_indices.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>
#include <sylk/core/utils/all.hpp>
#include <sylk/vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <limits>
#include <algorithm>

constexpr sylk::u32 U32_LIMIT = std::numeric_limits<sylk::u32>::max();
constexpr sylk::i32 MAX_FRAMES_IN_FLIGHT = 2;

namespace sylk {
    Swapchain::Swapchain(const vk::Device& device)
            : device_(device)
            , graphics_pipeline_(device)
            , command_buffers_(MAX_FRAMES_IN_FLIGHT)
            , semaphores_img_available_(MAX_FRAMES_IN_FLIGHT)
            , semaphores_render_finished_(MAX_FRAMES_IN_FLIGHT)
            , fences_in_flight_(MAX_FRAMES_IN_FLIGHT)
            , current_frame_(0)
    {}

    void Swapchain::create(const vk::PhysicalDevice physical_device, GLFWwindow* window, const vk::SurfaceKHR surface) {
        log(ELogLvl::TRACE, "Creating swapchain...");

        physical_device_ = physical_device;
        window_ = window;
        surface_ = surface;

        setup_swapchain();
        create_image_views();
        create_renderpass();
        graphics_pipeline_.create(extent_, renderpass_);
        create_framebuffers();
        create_command_pool();
        create_vertex_buffer();
        create_command_buffer();
        create_synchronizers();

        log(ELogLvl::DEBUG, "Created swapchain");
    }

    void Swapchain::destroy() {
        device_.destroyCommandPool(command_pool_);
        log(ELogLvl::TRACE, "Destroyed command pool");

        device_.destroyBuffer(vertex_buffer_.get_vkbuffer());
        device_.freeMemory(vertex_buffer_.get_memory_handle());
        log(ELogLvl::TRACE, "Destroyed vertex buffer");

        graphics_pipeline_.destroy();

        device_.destroyRenderPass(renderpass_);
        log(ELogLvl::TRACE, "Destroyed renderpass");

        for (u64 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            device_.destroySemaphore(semaphores_img_available_[i]);
            device_.destroySemaphore(semaphores_render_finished_[i]);
            device_.destroyFence(fences_in_flight_[i]);
        }
        log(ELogLvl::TRACE, "Destroyed synchronization objects");

        destroy_partial();

    }

    auto Swapchain::query_device_support_details(const vk::PhysicalDevice device, const vk::SurfaceKHR surface) const
    -> Swapchain::SupportDetails {
        log(ELogLvl::TRACE, "Querying swapchain support details...");

        SupportDetails details;

        const auto [cap_result, capabilities] = device.getSurfaceCapabilitiesKHR(surface);
        handle_result(cap_result, "Failed to acquire surface capabilities");
        details.surface_capabilities = capabilities;

        const auto [format_result, formats] = device.getSurfaceFormatsKHR(surface);
        handle_result(format_result, "Failed to acquire surface formats");
        details.surface_formats = formats;

        const auto [mode_result, modes] = device.getSurfacePresentModesKHR(surface);
        handle_result(mode_result, "Failed to acquire surface present modes");
        details.present_modes = modes;

        return details;
    }

    auto Swapchain::select_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) const -> vk::SurfaceFormatKHR {
        log(ELogLvl::TRACE, "Selecting swapchain surface format...");

        for (const auto& format : available_formats) {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return format;
            }
        }

        return available_formats.front();
    }

    auto Swapchain::select_present_mode(const std::vector<vk::PresentModeKHR>& available_modes) const -> vk::PresentModeKHR {
        log(ELogLvl::TRACE, "Selecting swapchain present mode...");

        for (const auto& mode : available_modes) {
            if (mode == vk::PresentModeKHR::eImmediate) {
                return mode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    auto Swapchain::select_extent_2d(const vk::SurfaceCapabilitiesKHR capabilities, GLFWwindow* window) const -> vk::Extent2D {
        log(ELogLvl::TRACE, "Selecting swapchain extent...");

        if (capabilities.currentExtent.width != U32_LIMIT) {
            return capabilities.currentExtent;
        }

        i32 window_width, window_height;
        glfwGetFramebufferSize(window, &window_width, &window_height);

        vk::Extent2D actual_extent {
                cast<u32>(window_width),
                cast<u32>(window_height)
        };

        actual_extent.width = std::clamp(actual_extent.width,
                                         capabilities.minImageExtent.width,
                                         capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height,
                                          capabilities.minImageExtent.height,
                                          capabilities.maxImageExtent.height);

        return actual_extent;
    }

    void Swapchain::create_image_views() {
        image_views_.resize(images_.size());

        const auto component_mappings = vk::ComponentMapping {
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity
        };

        const auto subresource_range = vk::ImageSubresourceRange {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        };

        for (u64 i = 0; i < images_.size(); ++i) {
            const auto create_info = vk::ImageViewCreateInfo {
                .image            = images_[i],
                .viewType         = vk::ImageViewType::e2D,
                .format           = format_,
                .components       = component_mappings,
                .subresourceRange = subresource_range,
            };

            const auto [result, view] = device_.createImageView(create_info);
            handle_result(result, "Failed to create image view");
            image_views_[i] = view;
        }

        log(ELogLvl::TRACE, "Created swapchain image views");
    }

    void Swapchain::create_framebuffers() {
        frame_buffers_.resize(image_views_.size());

        for (u64 i = 0; i < image_views_.size(); ++i) {
            const auto framebuffer_info = vk::FramebufferCreateInfo {
                .renderPass = renderpass_,
                .width      = extent_.width,
                .height     = extent_.height,
                .layers     = 1,
            }
            .setAttachments(image_views_[i]);

            const auto [result, buffer] = device_.createFramebuffer(framebuffer_info);
            handle_result(result, "Failed to create framebuffer");
            frame_buffers_[i] = buffer;
        }

        log(ELogLvl::TRACE, "Created framebuffers");
    }

    void Swapchain::create_synchronizers() {
        for (u64 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            const auto [sema_result_a, sema_a] = device_.createSemaphore(vk::SemaphoreCreateInfo());
            handle_result(sema_result_a, "Failed to create semaphore");

            const auto [sema_result_b, sema_b] = device_.createSemaphore(vk::SemaphoreCreateInfo());
            handle_result(sema_result_b, "Failed to create semaphore");

            const auto [fence_result, fence] = device_.createFence(vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
            handle_result(fence_result, "Failed to create fence");

            semaphores_img_available_[i] = sema_a;
            semaphores_render_finished_[i] = sema_b;
            fences_in_flight_[i] = fence;
        }

        log(ELogLvl::TRACE, "Created synchronizer objects");
    }

    void Swapchain::draw_next() {
        handle_result(device_.waitForFences(fences_in_flight_[current_frame_], true, UINT64_MAX), "Vulkan fence error");

        const auto [result, img_index] = device_.acquireNextImageKHR(swapchain_, UINT64_MAX, semaphores_img_available_[current_frame_]);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreate();
            return;
        } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            handle_result(result, "Image acquisition failed");
        }

        device_.resetFences(fences_in_flight_[current_frame_]);
        command_buffers_[current_frame_].reset();
        record_command_buffer(command_buffers_[current_frame_], img_index);

        const vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        const auto submit_info = vk::SubmitInfo()
                .setWaitSemaphores(semaphores_img_available_[current_frame_])
                .setWaitDstStageMask(wait_stage)
                .setSignalSemaphores(semaphores_render_finished_[current_frame_])
                .setCommandBuffers(command_buffers_[current_frame_]);

        handle_result(graphics_queue_.submit(submit_info, fences_in_flight_[current_frame_]),
                      "Failed to submit to graphics queue");

        const auto present_info = vk::PresentInfoKHR()
                .setWaitSemaphores(semaphores_render_finished_[current_frame_])
                .setSwapchains(swapchain_)
                .setImageIndices(img_index);

        vk::Result present_result = presentation_queue_.presentKHR(present_info);
        if (present_result == vk::Result::eErrorOutOfDateKHR || present_result == vk::Result::eSuboptimalKHR) {
            recreate();
        } else {
            handle_result(present_result, "Failed to present image");
        }

        current_frame_ = ++current_frame_ % MAX_FRAMES_IN_FLIGHT;
    }

    void Swapchain::record_command_buffer(const vk::CommandBuffer buffer, const u32 image_index) {
        const auto buffer_begin_info = vk::CommandBufferBeginInfo();
        handle_result(buffer.begin(buffer_begin_info), "Failed to start recording command buffer");

        const auto clear_color = vk::ClearValue {
            .color = { std::array {
                    0.0f, 0.0f, 0.0f, 1.0f
            }}
        };
        const auto renderpass_begin_info = vk::RenderPassBeginInfo {
            .renderPass  = renderpass_,
            .framebuffer = frame_buffers_[image_index],
            .renderArea  = vk::Rect2D {.extent = extent_},

        }
        .setClearValues(clear_color);

        buffer.beginRenderPass(renderpass_begin_info, vk::SubpassContents::eInline);
        buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline_.get_handle());

        buffer.bindVertexBuffers(0, vertex_buffer_.get_vkbuffer(), vk::DeviceSize{0});

        buffer.setViewport(0, vk::Viewport {
                .width    = cast<f32>(extent_.width),
                .height   = cast<f32>(extent_.height),
                .maxDepth = 1.0f,
        });

        buffer.setScissor(0, vk::Rect2D{.extent = extent_});

        buffer.draw(cast<u32>(vertices_.size()), 1, 0, 0);

        buffer.endRenderPass();
        handle_result(buffer.end(), "Failed to finish recording command buffer");
    }

    void Swapchain::create_command_pool() {
        const auto pool_info  = vk::CommandPoolCreateInfo {
            .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = graphics_queue_family_index_,
        };

        const auto [result, pool] = device_.createCommandPool(pool_info);
        handle_result(result, "Failed to create command pool");
        command_pool_ = pool;

        log(ELogLvl::TRACE, "Created command pool");
    }

    void Swapchain::create_command_buffer() {
        const auto buffer_alloc_info = vk::CommandBufferAllocateInfo {
                .commandPool        = command_pool_,
                .level              = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = cast<u32>(command_buffers_.size()),
        };

        const auto [result, buffer] = device_.allocateCommandBuffers(buffer_alloc_info);
        handle_result(result, "Failed to allocate command buffer");
        command_buffers_ = buffer;

        log(ELogLvl::TRACE, "Created command buffer");
    }

    void Swapchain::create_renderpass() {
        const auto color_attachment = vk::AttachmentDescription {
            .format         = format_,
            .samples        = vk::SampleCountFlagBits::e1,
            .loadOp         = vk::AttachmentLoadOp::eClear,
            .storeOp        = vk::AttachmentStoreOp::eStore,
            .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout  = vk::ImageLayout::eUndefined,
            .finalLayout    = vk::ImageLayout::ePresentSrcKHR,
        };

        const auto color_attachment_ref = vk::AttachmentReference {
            .attachment = 0,
            .layout     = vk::ImageLayout::eColorAttachmentOptimal,
        };

        const auto subpass = vk::SubpassDescription {
            .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        }
        .setColorAttachments(color_attachment_ref);

        const auto subpass_dependency = vk::SubpassDependency {
            .srcSubpass    = VK_SUBPASS_EXTERNAL,
            .dstSubpass    = 0,
            .srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .srcAccessMask = vk::AccessFlagBits::eNone,
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        };

        const auto render_pass_info = vk::RenderPassCreateInfo()
                .setAttachments(color_attachment)
                .setSubpasses(subpass)
                .setDependencies(subpass_dependency);

        const auto [result, renderpass] = device_.createRenderPass(render_pass_info);
        handle_result(result, "Failed to create renderpass");
        renderpass_ = renderpass;

        log(ELogLvl::TRACE, "Created render pass");
    }

    void Swapchain::set_queues(const vk::Queue graphics, const vk::Queue present) {
        graphics_queue_     = graphics;
        presentation_queue_ = present;
    }

    void Swapchain::destroy_partial() {
        for (auto framebuffer : frame_buffers_) {
            device_.destroyFramebuffer(framebuffer);
        }
        log(ELogLvl::TRACE, "Destroyed framebuffers");

        for (auto view : image_views_) {
            device_.destroyImageView(view);
        }
        log(ELogLvl::TRACE, "Destroyed image views");

        device_.destroySwapchainKHR(swapchain_);
        log(ELogLvl::TRACE, "Destroyed swapchain");
    }

    void Swapchain::recreate() {
        handle_result(device_.waitIdle(), "Device error");

        destroy_partial();

        setup_swapchain();
        create_image_views();
        create_framebuffers();

        log(ELogLvl::TRACE, "Re-created swapchain");
    }

    void Swapchain::setup_swapchain() {
        const auto support_details = query_device_support_details(physical_device_, surface_);
        const auto surface_format  = select_surface_format(support_details.surface_formats);

        format_ = surface_format.format;
        extent_ = select_extent_2d(support_details.surface_capabilities, window_);

        const auto max_image_count = support_details.surface_capabilities.maxImageCount;
        const auto min_image_count = support_details.surface_capabilities.minImageCount + 1;

        const auto swapchain_img_count = (max_image_count > 0 && min_image_count > max_image_count ?
                                            max_image_count : min_image_count);

        const auto queue_indices = QueueFamilyIndices::find(physical_device_, surface_);
        graphics_queue_family_index_ = queue_indices.graphics.value();
        std::vector active_queues {graphics_queue_family_index_, queue_indices.presentation.value()};
        const bool queues_equal = queue_indices.graphics == queue_indices.presentation;

        // exclusive mode does not require specified queue families
        if (!queues_equal) {
            active_queues.clear();
        }

        auto create_info = vk::SwapchainCreateInfoKHR {
            .surface          = surface_,
            .minImageCount    = swapchain_img_count,
            .imageFormat      = format_,
            .imageColorSpace  = surface_format.colorSpace,
            .imageExtent      = extent_,
            .imageArrayLayers = 1,
            .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = (queues_equal ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent),
            .preTransform     = support_details.surface_capabilities.currentTransform,
            .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode      = select_present_mode(support_details.present_modes),
            .clipped          = true,
            .oldSwapchain     = nullptr,
        }
        .setQueueFamilyIndices(active_queues);

        const auto [swapc_result, swapchain] = device_.createSwapchainKHR(create_info, nullptr);
        handle_result(swapc_result, "Failed to create swapchain");
        swapchain_ = swapchain;

        const auto [img_result, images] = device_.getSwapchainImagesKHR(swapchain_);
        handle_result(img_result, "Failed to acquire swapchain images");
        images_ = images;
    }

    void Swapchain::create_vertex_buffer() {
        const auto buffer_data = Buffer::CreateData {
                .data_to_map        = vertices_.data(),
                .device             = device_,
                .physical_device    = physical_device_,
                .buffer_size        = sizeof(vertices_[0]) * vertices_.size(),
                .buffer_usage_flags = vk::BufferUsageFlagBits::eVertexBuffer,
                .property_flags     = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
        };

        vertex_buffer_.create(buffer_data);
    }

}