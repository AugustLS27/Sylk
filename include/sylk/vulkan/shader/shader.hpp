//
// Created by August Silva on 9-3-23.
//

#pragma once

#include <sylk/core/utils/rust_style_types.hpp>
#include <sylk/vulkan/vulkan.hpp>

#include <vector>

namespace sylk {
    class Shader {

    public:
        Shader(const vk::Device& device);
        void create(const char* filename);
        void destroy();

        auto get_module() const -> vk::ShaderModule;

    private:
        void create_shader_module();

    private:
        const vk::Device& device_;
        std::vector<char> shader_code_;
        vk::ShaderModule shader_module_;
    };
}
