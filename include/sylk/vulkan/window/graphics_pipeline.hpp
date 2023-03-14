//
// Created by August Silva on 8-3-23.
//

#ifndef SYLK_VULKAN_WINDOW_GRAPHICSPIPELINE_HPP
#define SYLK_VULKAN_WINDOW_GRAPHICSPIPELINE_HPP

#include <sylk/vulkan/shader/shader.hpp>
#include <sylk/vulkan/vulkan.hpp>

namespace sylk {

    class GraphicsPipeline {
      public:
        GraphicsPipeline(const vk::Device& device);
        void create(vk::Extent2D extent, vk::RenderPass renderpass);
        void destroy() const;
        void destroy_descriptorset_layouts();

        auto get_layout() const -> vk::PipelineLayout;
        auto get_handle() const -> vk::Pipeline;
        auto get_descriptor_set_layout() const -> vk::DescriptorSetLayout;

      private:
        void create_descriptorset_layout();

      private:
        const vk::Device&       device_;
        vk::DescriptorSetLayout descriptor_set_layout_;
        vk::PipelineLayout      layout_;
        vk::Pipeline            pipeline_;
        Shader                  vertex_shader_;
        Shader                  fragment_shader_;
    };

}

#endif  // SYLK_VULKAN_WINDOW_GRAPHICSPIPELINE_HPP