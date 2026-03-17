module;
#include "precompiled.h"

export module lxlib.GlslProg;

export class GlslProg {
public:
    GlslProg(const std::string& fragmentSrc, const std::string& vertexSrc)
    {
        GLuint vs = compile(GL_VERTEX_SHADER, vertexSrc);
        GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSrc);

        m_program = glCreateProgram();
        if (!m_program)
            throw std::runtime_error("glCreateProgram failed");

        glAttachShader(m_program, vs);
        glAttachShader(m_program, fs);
        glLinkProgram(m_program);

        GLint success = 0;
        glGetProgramiv(m_program, GL_LINK_STATUS, &success);
        if (!success) {
            GLint logLen = 0;
            glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLen);

            std::vector<char> log(logLen);
            glGetProgramInfoLog(m_program, logLen, nullptr, log.data());

            glDeleteShader(vs);
            glDeleteShader(fs);
            glDeleteProgram(m_program);

            throw std::runtime_error("Program link error:\n" + std::string(log.data()));
        }

        glDetachShader(m_program, vs);
        glDetachShader(m_program, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    ~GlslProg() {
        if (m_program)
            glDeleteProgram(m_program);
    }

    void bind() const {
        glUseProgram(m_program);
    }

    static void unbind() {
        glUseProgram(0);
    }

    GLuint id() const { return m_program; }

    GLint getUniformLocation(const std::string& name) const {
        return glGetUniformLocation(m_program, name.c_str());
    }

    void uniform(const std::string& name, int v) const {
        glUniform1i(getUniformLocation(name), v);
    }

    void uniform(const std::string& name, float v) const {
        glUniform1f(getUniformLocation(name), v);
    }

    void uniform(const std::string & name, glm::vec2 v) const {
        glUniform2fv(getUniformLocation(name), 1, &v[0]);
    }

private:
    GLuint m_program = 0;

    static GLuint compile(GLenum type, const std::string& src)
    {
        GLuint shader = glCreateShader(type);
        if (!shader)
            throw std::runtime_error("glCreateShader failed");

        const char* cstr = src.c_str();
        glShaderSource(shader, 1, &cstr, nullptr);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint logLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);

            std::vector<char> log(logLen);
            glGetShaderInfoLog(shader, logLen, nullptr, log.data());

            glDeleteShader(shader);

            throw std::runtime_error("Shader compile error:\n" + std::string(log.data()));
        }

        return shader;
    }
};

export typedef std::shared_ptr<GlslProg> GlslProgRef;
