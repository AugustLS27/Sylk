//
// Created by August Silva on 8-3-23.
//

#include <sylk/vulkan/window/graphics_pipeline.hpp>
#include <sylk/core/utils/all.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>

constexpr const char* DEFAULT_SHADER_ENTRY_NAME = "main";

namespace sylk {
    void GraphicsPipeline::create(const vk::Extent2D extent, const vk::RenderPass renderpass) {
        vertex_shader_.create("../../shaders/vk/spv/vert.spv");
        fragment_shader_.create("../../shaders/vk/spv/frag.spv");

        const auto vert_stage_info = vk::PipelineShaderStageCreateInfo {
                .stage  = vk::ShaderStageFlagBits::eVertex,
                .module = vertex_shader_.get_module(),
                .pName  = DEFAULT_SHADER_ENTRY_NAME,
        };

        const auto frag_stage_info = vk::PipelineShaderStageCreateInfo {
                .stage  = vk::ShaderStageFlagBits::eFragment,
                .module = fragment_shader_.get_module(),
                .pName  = DEFAULT_SHADER_ENTRY_NAME,
        };

        const std::array shader_stages = {
                vert_stage_info,
                frag_stage_info,
        };

        const std::vector<vk::DynamicState> dynamic_states = {
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor,
        };

        const auto dynamic_state_info = vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamic_states);

        const auto vertex_input_state_info = vk::PipelineVertexInputStateCreateInfo {
                .vertexBindingDescriptionCount   = 0,
                .vertexAttributeDescriptionCount = 0,
        };

        const auto input_assembly_state_info = vk::PipelineInputAssemblyStateCreateInfo {
                .topology               = vk::PrimitiveTopology::eTriangleList,
                .primitiveRestartEnable = false,
        };

        const auto viewport = vk::Viewport {
                .width    = cast<f32>(extent.width),
                .height   = cast<f32>(extent.height),
                .maxDepth = 1.0f,
        };

        const auto scissor = vk::Rect2D {
            .extent = extent
        };

        const auto viewport_state_info = vk::PipelineViewportStateCreateInfo()
                .setViewports(viewport)
                .setScissors(scissor);

        const auto rasterizer_info = vk::PipelineRasterizationStateCreateInfo {
                .depthClampEnable        = false,
                .rasterizerDiscardEnable = false,
                .polygonMode             = vk::PolygonMode::eFill,
                .cullMode                = vk::CullModeFlagBits::eBack,
                .frontFace               = vk::FrontFace::eClockwise,
                .depthBiasEnable         = false,
                .lineWidth               = 1.0f,
        };

        const auto multisampling_info = vk::PipelineMultisampleStateCreateInfo {
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable  = true,
        };

        const auto color_blend_attachment = vk::PipelineColorBlendAttachmentState {
                .blendEnable         = true,
                .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
                .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
                .colorBlendOp        = vk::BlendOp::eAdd,
                .srcAlphaBlendFactor = vk::BlendFactor::eOne,
                .dstAlphaBlendFactor = vk::BlendFactor::eZero,
                .alphaBlendOp        = vk::BlendOp::eAdd,
                .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                        | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };

        const auto color_blend_info = vk::PipelineColorBlendStateCreateInfo()
                .setLogicOpEnable(false)
                .setAttachments(color_blend_attachment);

        const auto layout_info = vk::PipelineLayoutCreateInfo();
        const auto [layout_result, layout] = device_.createPipelineLayout(layout_info);
        handle_result(layout_result, "Failed to create pipeline layout", ELogLvl::ERROR);
        layout_ = layout;

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

        const auto [result, value] = device_.createGraphicsPipeline(nullptr, pipeline_info);
        handle_result(result, "Failed to create graphics pipeline", ELogLvl::CRITICAL);
        pipeline_ = value;

        vertex_shader_.destroy();
        fragment_shader_.destroy();

        log(ELogLvl::DEBUG, "Created graphics pipeline");
    }

    GraphicsPipeline::GraphicsPipeline(const vk::Device& device)
            : device_(device), fragment_shader_(device), vertex_shader_(device) {}

    void GraphicsPipeline::destroy() const {
        device_.destroyPipeline(pipeline_);
        log(ELogLvl::TRACE, "Destroyed graphics pipeline");

        device_.destroyPipelineLayout(layout_);
        log(ELogLvl::TRACE, "Destroyed graphics pipeline layout");
    }

    auto GraphicsPipeline::get_handle() const -> vk::Pipeline {
        return pipeline_;
    }
}