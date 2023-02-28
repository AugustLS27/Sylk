//
// Created by August on 26-2-23.
//

#include <glad/glad.h>

#include <sylk/shader/program.hpp>
#include <sylk/coreutils/all.hpp>

namespace sylk {

    Program::Program()
        : handle_(glCreateProgram()){
    }

    Program::~Program() {
        glDeleteProgram(handle_);
    }

    void Program::add_shader(u32 shader) const {
        glAttachShader(handle_, shader);
    }

    void Program::add_shader(Shader& shader) const {
        glAttachShader(handle_, shader.handle());
    }

    void Program::add_shader(Shader::Type type, const char* filename, const char* path_prefix) const {
        Shader shader(type, filename, path_prefix);
        glAttachShader(handle_, shader.handle());
    }

    void Program::link() const {
        glLinkProgram(handle_);

        int success;
        glGetProgramiv(handle_, GL_LINK_STATUS, &success);
        if (!success) {
            char info_log[1024];
            glGetProgramInfoLog(handle_, 1024, nullptr, info_log);

            log(CRITICAL, "Shader program linking failed:\n{}", info_log);
        }
    }

    void Program::use() const {
        glUseProgram(handle_);
    }

    void Program::detach_shader(u32 shader) const {
        glDetachShader(handle_, shader);
    }

    void Program::detach_shader(Shader& shader) const {
        glDetachShader(handle_, shader.handle());
    }

    u32 Program::get_handle() const {
        return handle_;
    }
}
