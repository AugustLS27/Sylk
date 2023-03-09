//
// Created by August on 25-2-23.
//

#include <glad/glad.h>

#include <sylk/core/utils/all.hpp>
#include <sylk/opengl/shader/shader.hpp>

#include <fstream>
#include <string>

namespace sylk {
    const char* Shader::DEFAULT_PATH_PREFIX_ = "../../shaders/";

    Shader::Shader(Shader::Type type, const char* filename, const char* path_prefix)
        : type_(type)
        , prefix_(path_prefix)
        , handle_(0) {
        load(filename);
    }

    Shader::Shader(Shader::Type type)
        : type_(type)
        , prefix_(DEFAULT_PATH_PREFIX_)
        , handle_(0)
        {}

    Shader::~Shader() {
        glDeleteShader(handle_);
    }

    void Shader::create() {
        switch (type_) {
            using enum Shader::Type;

            case Vertex:
                handle_ = glCreateShader(GL_VERTEX_SHADER);
                break;
            case Fragment:
                handle_ = glCreateShader(GL_FRAGMENT_SHADER);
                break;
            case Geometry:
                handle_ = glCreateShader(GL_GEOMETRY_SHADER);
                break;
            case TessControl:
#ifdef GL_TESS_CONTROL_SHADER
                handle_ = glCreateShader(GL_TESS_CONTROL_SHADER);
#else
                log(ERROR, "Tesselation control shader requested, but not found.");
#endif
                break;
            case TessEval:
#ifdef GL_TESS_EVALUATION_SHADER
                handle_ = glCreateShader(GL_TESS_EVALUATION_SHADER);
#else
                log(ERROR, "Tesselation evaluation shader requested, but not found.");
#endif
                break;
            case Compute:
#ifdef GL_COMPUTE_SHADER
                handle_ = glCreateShader(GL_COMPUTE_SHADER);
#else
                log(ERROR, "Compute shader is unavailable for this version of OpenGL.");
#endif
                break;
        }
    }

    u32 Shader::handle() const {
        return handle_;
    }

    Shader::Type Shader::type() const {
        return type_;
    }

    void Shader::load(const char* filename) {
        std::ifstream file(std::string(prefix_) + filename);
        if (!file.is_open()) {
            log(CRITICAL, "Failed to read vertex shader");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content_str = buffer.str();

        create();

        auto src = content_str.c_str();
        glShaderSource(handle_, 1, &src, nullptr);
        glCompileShader(handle_);

        int success{};
        glGetShaderiv(handle_, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info_log[1024];
            glGetShaderInfoLog(handle_, 1024, nullptr, info_log);

            log(ERROR, "Shader compilation failed:\n{}", info_log);
        }
    }

    void Shader::set_default_path_prefix(const char* path) {
        log(WARN, "Default shader path prefix updated to \"{}\"", path);
        DEFAULT_PATH_PREFIX_ = path;
    }

    void Shader::set_path_prefix(const char* path) {
        prefix_ = path;
    }

    const char* Shader::default_path_prefix() {
        return DEFAULT_PATH_PREFIX_;
    }
}
