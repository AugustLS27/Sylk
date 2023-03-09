//
// Created by August on 26-2-23.
//

#pragma once

#include <sylk/coreutils/rust_style_types.hpp>
#include <sylk/shader/shader.hpp>

#include <vector>

namespace sylk {

    class Program {
        const u32 handle_;
    public:
        Program();
        ~Program();

        [[nodiscard]] u32 get_handle() const;

        void add_shader(u32 handle) const;
        void add_shader(Shader& shader) const;
        void add_shader(Shader::Type type, const char* filename, const char* path_prefix = Shader::default_path_prefix()) const;

        void detach_shader(u32 handle) const;
        void detach_shader(Shader& shader) const;

        void link() const;
        void use() const;
    };

}
