#ifndef SHADER_H
#define SHADER_H

#include "gl_loader.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

enum ShaderType {
    VertexShader = GL_VERTEX_SHADER,
    TessControlShader = GL_TESS_CONTROL_SHADER,
    TessEvaluationShader = GL_TESS_EVALUATION_SHADER,
    GeometryShader = GL_GEOMETRY_SHADER,
    FragmentShader = GL_FRAGMENT_SHADER
};

class GLShader {
public:
    GLuint id = 0;
    ShaderType type;

    GLShader() : id(0), type(VertexShader) {}

    GLShader(ShaderType _type, const std::string& filepath) : type(_type) {
        std::string resolved_path = resolve_path(filepath);
        std::ifstream file(resolved_path);
        if (!file.is_open()) {
            std::cerr << "[GLShader] Failed to open shader file: " << filepath << " (resolved as: " << resolved_path << ")\n";
            return;
        }

        std::stringstream ss;
        ss << file.rdbuf();
        std::string source = ss.str();
        compile(source, resolved_path);
    }

    static GLShader from_source(ShaderType _type, const std::string& source) {
        GLShader s;
        s.type = _type;
        s.compile(source, "inline_source");
        return s;
    }

private:
    static std::string resolve_path(const std::string& path) {
        // Try original path
        std::ifstream test(path);
        if (test.good()) return path;

        // Try just the basename in current dir
        size_t last_slash = path.find_last_of("/\\");
        std::string basename = (last_slash != std::string::npos) ? path.substr(last_slash + 1) : path;
        std::ifstream test_base(basename);
        if (test_base.good()) return basename;

        // Try in parent dir (if running from build/)
        std::string parent_base = "../" + basename;
        std::ifstream test_parent_base(parent_base);
        if (test_parent_base.good()) return parent_base;

        std::string parent_path = "../" + path;
        std::ifstream test_parent_path(parent_path);
        if (test_parent_path.good()) return parent_path;

        return path;
    }

    void compile(const std::string& source, const std::string& desc) {
        id = glCreateShader(type);
        const char* src_ptr = source.c_str();
        glShaderSource(id, 1, &src_ptr, nullptr);
        glCompileShader(id);

        GLint success = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint log_length = 0;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
            std::vector<char> info_log(log_length > 0 ? log_length : 512);
            glGetShaderInfoLog(id, (GLsizei)info_log.size(), nullptr, info_log.data());
            std::cerr << "[GLShader] Compilation error in " << desc << ":\n" << info_log.data() << "\n";
        }
    }
};

class GLShaderProgram {
public:
    GLuint id = 0;
    std::vector<GLuint> attached_shaders;

    GLShaderProgram() {
        id = glCreateProgram();
    }

    ~GLShaderProgram() {
        if (id) {
            glDeleteProgram(id);
            id = 0;
        }
    }

    void add_shader(const GLShader& shader) {
        if (shader.id) {
            glAttachShader(id, shader.id);
            attached_shaders.push_back(shader.id);
        }
    }

    bool link() {
        glLinkProgram(id);

        GLint success = 0;
        glGetProgramiv(id, GL_LINK_STATUS, &success);
        if (!success) {
            GLint log_length = 0;
            glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
            std::vector<char> info_log(log_length > 0 ? log_length : 512);
            glGetProgramInfoLog(id, (GLsizei)info_log.size(), nullptr, info_log.data());
            std::cerr << "[GLShaderProgram] Link error:\n" << info_log.data() << "\n";
            return false;
        }

        // Clean up attached shader objects once program is linked
        for (GLuint s_id : attached_shaders) {
            glDetachShader(id, s_id);
            glDeleteShader(s_id);
        }
        attached_shaders.clear();
        return true;
    }

    void bind() const {
        glUseProgram(id);
    }

    void unbind() const {
        glUseProgram(0);
    }

    GLint uniform_location(const char* name) const {
        return glGetUniformLocation(id, name);
    }

    GLint uniform_location(const std::string& name) const {
        return glGetUniformLocation(id, name.c_str());
    }
};

#endif // SHADER_H
