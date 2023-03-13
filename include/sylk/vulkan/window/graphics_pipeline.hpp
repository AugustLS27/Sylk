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

        auto get_handle() const -> vk::Pipeline;

      private:
        const vk::Device&  device_;
        vk::PipelineLayout layout_;
        vk::Pipeline       pipeline_;
        Shader             vertex_shader_;
        Shader             fragment_shader_;
    };

}

#endif  // SYLK_VULKAN_WINDOW_GRAPHICSPIPELINE_HPP