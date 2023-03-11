//
// Created by August Silva on 8-3-23.
//

#pragma once

#include <sylk/vulkan/vulkan.hpp>
#include <sylk/vulkan/shader/shader.hpp>

namespace sylk {

    class GraphicsPipeline {
    public:
        GraphicsPipeline(const vk::Device& device);
        void create(vk::Extent2D extent, vk::RenderPass renderpass);
        void destroy() const;

        auto get_handle() const -> vk::Pipeline;

    private:
        const vk::Device& device_;
        vk::PipelineLayout layout_;
        vk::Pipeline pipeline_;
        Shader vertex_shader_;
        Shader fragment_shader_;
    };

}
