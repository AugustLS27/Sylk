//
// Created by August Silva on 9-3-23.
//

#include <sylk/vulkan/shader/shader.hpp>
#include <sylk/core/utils/all.hpp>

#include <fstream>

namespace sylk {
    Shader::Shader(const vk::Device& device)
        : device_(device)
        {}

    void Shader::create(const char* filename) {
        // start at end of file so size is easily known
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (not file.is_open()) {
            log(CRITICAL, "Failed to open shader file: {}", filename);
        }

        u64 file_size = cast<u64>(file.tellg());
        shader_code_.resize(file_size);

        // go back to start of file for reading
        file.seekg(0);
        file.read(shader_code_.data(), file_size);

        create_shader_module();
    }

    void Shader::create_shader_module() {
        const auto create_info = vk::ShaderModuleCreateInfo()
                .setPCode(reinterpret_cast<const u32*>(shader_code_.data()))
                .setCodeSize(shader_code_.size());
        shader_module_ = device_.createShaderModule(create_info);
    }

    void Shader::destroy() {
        device_.destroyShaderModule(shader_module_);
    }

    vk::ShaderModule Shader::get_module() const {
        return shader_module_;
    }
}
