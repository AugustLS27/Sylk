//
// Created by August Silva on 8-3-23.
//

#pragma once

#include <sylk/vulkan/shader/shader.hpp>

#include <vulkan/vulkan.hpp>

namespace sylk {

    class GraphicsPipeline {
    public:
        GraphicsPipeline(const vk::Device& device);
        void create(vk::Extent2D extent);
        void destroy() const;

    private:

    private:
        const vk::Device& device_;
        vk::PipelineLayout layout_;
        Shader vertex_shader_;
        Shader fragment_shader_;
    };

}
