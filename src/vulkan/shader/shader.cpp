//
// Created by August Silva on 9-3-23.
//

#include <sylk/core/utils/all.hpp>
#include <sylk/vulkan/shader/shader.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>

#include <fstream>

namespace sylk {
    Shader::Shader(const vk::Device& device)
        : device_(device) {}

    void Shader::create(const char* filename) {
        // start at end of file so size is easily known
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (not file.is_open()) {
            log(ELogLvl::CRITICAL, "Failed to open shader file: {}", filename);
        }

        u64 file_size = cast<u64>(file.tellg());
        shader_code_.resize(file_size);

        // go back to start of file for reading
        file.seekg(0);
        file.read(shader_code_.data(), file_size);

        create_shader_module();
    }

    void Shader::create_shader_module() {
        // casting from char is well defined, so this does not break strict
        // aliasing rules alignment is also not an issue due to how vector
        // allocates its data also, vulkan guarantees the endianness of the host
        // and device are the same, and the sorts of machines that sylk targets
        // are highly likely to be in the expected endianness
        const auto create_info = vk::ShaderModuleCreateInfo {
            .codeSize = shader_code_.size(),
            .pCode    = reinterpret_cast<const u32*>(shader_code_.data()),
        };

        const auto [result, shader] = device_.createShaderModule(create_info);
        handle_result(result, "Failed to create shader module");
        shader_module_ = shader;
    }

    void Shader::destroy() { device_.destroyShaderModule(shader_module_); }

    auto Shader::get_module() const -> vk::ShaderModule { return shader_module_; }
}
