//
// Created by August on 25-2-23.
//

#pragma once

#include <sylk/coreutils/rust_style_types.hpp>

namespace sylk {

    class Shader {
    public:
        enum class Type {
            Vertex,
            Fragment,
            Geometry,
            TessControl, // GL 4.0 / GL_ARB_tesselation_shader
            TessEval,    // ^
            Compute,     // GL 4.3 / GL_ARB_compute_shader
        };

    private:
        u32 handle_;
        Type type_;
        const char* prefix_;

        static const char* DEFAULT_PATH_PREFIX_;

        void create();

    public:
        Shader(Type type, const char* filename, const char* path_prefix = DEFAULT_PATH_PREFIX_);
        explicit Shader(Type type);
        ~Shader();

        static void set_default_path_prefix(const char* path);
        static const char* default_path_prefix();

        void set_path_prefix(const char* path);
        void load(const char* filename);

        [[nodiscard]] Type type() const;
        [[nodiscard]] u32 handle() const;
    };

}
