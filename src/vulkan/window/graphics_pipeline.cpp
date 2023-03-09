//
// Created by August Silva on 8-3-23.
//

#include <sylk/vulkan/window/graphics_pipeline.hpp>

#include <array>
#include "sylk/core/utils/cast.hpp"

constexpr const char* DEFAULT_SHADER_ENTRY_NAME = "main";

namespace sylk {
    void GraphicsPipeline::create(vk::Extent2D extent) {
        vertex_shader_.create("../../shaders/vk/spv/vert.spv");
        fragment_shader_.create("../../shaders/vk/spv/frag.spv");

        auto vert_stage_info = vk::PipelineShaderStageCreateInfo()
                .setModule(vertex_shader_.get_module())
                .setPName(DEFAULT_SHADER_ENTRY_NAME)
                .setStage(vk::ShaderStageFlagBits::eVertex);

        auto frag_stage_info = vk::PipelineShaderStageCreateInfo()
                .setModule(fragment_shader_.get_module())
                .setPName(DEFAULT_SHADER_ENTRY_NAME)
                .setStage(vk::ShaderStageFlagBits::eFragment);

        std::array shader_stages = {
                vert_stage_info,
                frag_stage_info,
        };

        std::vector<vk::DynamicState> dynamic_states = {
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor,
        };

        auto dynamic_state_info = vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamic_states);

        auto vertex_input_state_info = vk::PipelineVertexInputStateCreateInfo()
                .setVertexAttributeDescriptionCount(0)
                .setVertexBindingDescriptionCount(0);
        
        auto input_assembly_state_info = vk::PipelineInputAssemblyStateCreateInfo()
                .setTopology(vk::PrimitiveTopology::eTriangleList)
                .setPrimitiveRestartEnable(false);

        auto viewport = vk::Viewport({}, {}, cast<f32>(extent.width), cast<f32>(extent.height), {}, 1.f);

        auto scissor = vk::Rect2D({}, extent);

        auto viewport_state_info = vk::PipelineViewportStateCreateInfo()
                .setViewports(viewport)
                .setScissors(scissor);

        auto rasterizer = vk::PipelineRasterizationStateCreateInfo()
                .setDepthClampEnable(false)
                .setDepthBiasEnable(false)
                .setRasterizerDiscardEnable(false)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setLineWidth(1.0f)
                .setCullMode(vk::CullModeFlagBits::eBack)
                .setFrontFace(vk::FrontFace::eClockwise);

        auto multisampling = vk::PipelineMultisampleStateCreateInfo()
                .setSampleShadingEnable(false)
                .setRasterizationSamples(vk::SampleCountFlagBits::e1);

        auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
                .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                    | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
                .setBlendEnable(true)
                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                .setAlphaBlendOp(vk::BlendOp::eAdd);

        auto color_blending = vk::PipelineColorBlendStateCreateInfo()
                .setLogicOpEnable(false)
                .setAttachments(color_blend_attachment);

        auto layout_create_info = vk::PipelineLayoutCreateInfo();

        layout_ = device_.createPipelineLayout(layout_create_info);


        vertex_shader_.destroy();
        fragment_shader_.destroy();
    }

    GraphicsPipeline::GraphicsPipeline(const vk::Device& device)
        : device_(device)
        , fragment_shader_(device)
        , vertex_shader_(device) {}

    void GraphicsPipeline::destroy() const {
        device_.destroyPipelineLayout(layout_);
    }
}