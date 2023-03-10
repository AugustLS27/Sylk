//
// Created by August Silva on 8-3-23.
//

#include <sylk/vulkan/window/graphics_pipeline.hpp>
#include <sylk/core/utils/all.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>

constexpr const char* DEFAULT_SHADER_ENTRY_NAME = "main";

namespace sylk {
    void GraphicsPipeline::create(vk::Extent2D extent, vk::RenderPass renderpass) {
        vertex_shader_.create("../../shaders/vk/spv/vert.spv");
        fragment_shader_.create("../../shaders/vk/spv/frag.spv");

        const auto vert_stage_info = vk::PipelineShaderStageCreateInfo()
                .setModule(vertex_shader_.get_module())
                .setPName(DEFAULT_SHADER_ENTRY_NAME)
                .setStage(vk::ShaderStageFlagBits::eVertex);

        const auto frag_stage_info = vk::PipelineShaderStageCreateInfo()
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

        const auto dynamic_state_info = vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamic_states);

        const auto vertex_input_state_info = vk::PipelineVertexInputStateCreateInfo()
                .setVertexAttributeDescriptionCount(0)
                .setVertexBindingDescriptionCount(0);
        
        const auto input_assembly_state_info = vk::PipelineInputAssemblyStateCreateInfo()
                .setTopology(vk::PrimitiveTopology::eTriangleList)
                .setPrimitiveRestartEnable(false);

        const auto viewport = vk::Viewport({}, {}, cast<f32>(extent.width), cast<f32>(extent.height), {}, 1.f);

        const auto scissor = vk::Rect2D({}, extent);

        const auto viewport_state_info = vk::PipelineViewportStateCreateInfo()
                .setViewports(viewport)
                .setScissors(scissor);

        const auto rasterizer_info = vk::PipelineRasterizationStateCreateInfo()
                .setDepthClampEnable(false)
                .setDepthBiasEnable(false)
                .setRasterizerDiscardEnable(false)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setLineWidth(1.0f)
                .setCullMode(vk::CullModeFlagBits::eBack)
                .setFrontFace(vk::FrontFace::eClockwise);

        const auto multisampling_info = vk::PipelineMultisampleStateCreateInfo()
                .setSampleShadingEnable(false)
                .setRasterizationSamples(vk::SampleCountFlagBits::e1);

        const auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
                .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                    | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
                .setBlendEnable(true)
                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                .setAlphaBlendOp(vk::BlendOp::eAdd);

        const auto color_blend_info = vk::PipelineColorBlendStateCreateInfo()
                .setLogicOpEnable(false)
                .setAttachments(color_blend_attachment);

        const auto layout_info = vk::PipelineLayoutCreateInfo();
        layout_ = device_.createPipelineLayout(layout_info);

        const auto pipeline_info = vk::GraphicsPipelineCreateInfo()
                .setStages(shader_stages)
                .setPVertexInputState(&vertex_input_state_info)
                .setPInputAssemblyState(&input_assembly_state_info)
                .setPViewportState(&viewport_state_info)
                .setPRasterizationState(&rasterizer_info)
                .setPMultisampleState(&multisampling_info)
                .setPColorBlendState(&color_blend_info)
                .setPDynamicState(&dynamic_state_info)
                .setLayout(layout_)
                .setRenderPass(renderpass)
                .setSubpass(0);

        auto [result, value] = device_.createGraphicsPipeline(nullptr, pipeline_info);
        handle_result(result, "Failed to create graphics pipeline", true);
        pipeline_ = value;

        vertex_shader_.destroy();
        fragment_shader_.destroy();

        log(DEBUG, "Created graphics pipeline");
    }

    GraphicsPipeline::GraphicsPipeline(const vk::Device& device)
        : device_(device)
        , fragment_shader_(device)
        , vertex_shader_(device) {}

    void GraphicsPipeline::destroy() const {
        device_.destroyPipeline(pipeline_);
        log(TRACE, "Destroyed graphics pipeline");

        device_.destroyPipelineLayout(layout_);
        log(TRACE, "Destroyed graphics pipeline layout");
    }

    vk::Pipeline GraphicsPipeline::get() const {
        return pipeline_;
    }
}