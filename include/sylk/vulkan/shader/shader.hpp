//
// Created by August Silva on 9-3-23.
//

#pragma once

#include <sylk/core/utils/rust_style_types.hpp>

#include <vulkan/vulkan.hpp>

#include <vector>

namespace sylk {
    class Shader {

    public:
        Shader(const vk::Device& device);
        void create(const char* filename);
        void destroy();

        vk::ShaderModule get_module() const;

    private:
        void create_shader_module();

    private:
        const vk::Device& device_;
        std::vector<u32> shader_code_;
        vk::ShaderModule shader_module_;
    };
}
