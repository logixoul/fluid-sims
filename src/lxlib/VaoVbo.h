// Basic OpenGL (C++): VAO + two non-interleaved VBOs (position + texcoord), non-indexed quad.
#pragma once
#include <glad/glad.h>
#include <cstddef>   // offsetof
#include <cstdint>
#include <stdexcept>
class VBO {
public:
    VBO() {
        glGenBuffers(1, &m_id);
        if (!m_id) throw std::runtime_error("glGenBuffers failed");
    }

    ~VBO() {
        if (m_id) glDeleteBuffers(1, &m_id);
    }

    void bind(GLenum target = GL_ARRAY_BUFFER) const {
        m_target = target;
        glBindBuffer(target, m_id);
    }

    static void unbind(GLenum target = GL_ARRAY_BUFFER) {
        glBindBuffer(target, 0);
    }

    void setData(const void* data, GLsizeiptr bytes, GLenum usage = GL_STATIC_DRAW, GLenum target = GL_ARRAY_BUFFER) const {
        bind(target);
        glBufferData(target, bytes, data, usage);
    }

    GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;
    mutable GLenum m_target = GL_ARRAY_BUFFER;
};

// ----------------------------- VAO -----------------------------
class VAO {
public:
    VAO() {
        glGenVertexArrays(1, &m_id);
        if (!m_id) throw std::runtime_error("glGenVertexArrays failed");
    }

    ~VAO() {
        if (m_id) glDeleteVertexArrays(1, &m_id);
    }

    void bind() const { glBindVertexArray(m_id); }
    static void unbind() { glBindVertexArray(0); }

    // Helper: define an attribute pulling from currently-bound GL_ARRAY_BUFFER
    // NOTE: Requires this VAO to be bound.
    void defineAttrib(
        GLuint index,
        GLint  components,
        GLenum type,
        GLboolean normalized,
        GLsizei strideBytes,
        const void* offsetBytes
    ) const {
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, components, type, normalized, strideBytes, offsetBytes);
    }

    GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;
};

// ----------------------------- quad setup -----------------------------
// Layout assumption for your shader:
//   layout(location=0) in vec4 aPos;
//   layout(location=1) in vec2 aUV;
struct QuadGpu {
    VAO vao;
    VBO vboPos; // vec4 positions, non-interleaved
    VBO vboUV;  // vec2 texcoords, non-interleaved
};

inline std::shared_ptr<QuadGpu> createQuadVAO_VBOs()
{
    // Four hardcoded vertices, non-indexed. We'll draw as GL_TRIANGLE_FAN or GL_TRIANGLE_STRIP.
    // Order: (0,0)->(0,1)->(1,1)->(1,0)
    static const float positions[6][4] = {
        {0.f, 0.f, 0.f, 1.f}, // #1
        {0.f, 1.f, 0.f, 1.f}, // #2
        {1.f, 1.f, 0.f, 1.f}, // #3
        {0.f, 0.f, 0.f, 1.f}, // #1
        {1.f, 1.f, 0.f, 1.f}, // #3
        {1.f, 0.f, 0.f, 1.f}, // #2
    };

    static const float texcoords[6][2] = {
        {0.f, 0.f}, // #1
        {0.f, 1.f}, // #2
        {1.f, 1.f}, // #3
        {0.f, 0.f}, // #1
        {1.f, 1.f}, // #3
        {1.f, 0.f}, // #2
    };

    auto q = std::make_shared<QuadGpu>();

    q->vao.bind();

    // Attribute 0: positions (vec4) from its own VBO
    q->vboPos.setData(positions, sizeof(positions), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
    q->vboPos.bind(GL_ARRAY_BUFFER);
    q->vao.defineAttrib(
        /*index*/ 0,
        /*components*/ 4,
        /*type*/ GL_FLOAT,
        /*normalized*/ GL_FALSE,
        /*stride*/ 4 * (GLsizei)sizeof(float),
        /*offset*/ (const void*)0
    );

    // Attribute 1: texcoords (vec2) from its own VBO
    q->vboUV.setData(texcoords, sizeof(texcoords), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
    q->vboUV.bind(GL_ARRAY_BUFFER);
    q->vao.defineAttrib(
        /*index*/ 1,
        /*components*/ 2,
        /*type*/ GL_FLOAT,
        /*normalized*/ GL_FALSE,
        /*stride*/ 2 * (GLsizei)sizeof(float),
        /*offset*/ (const void*)0
    );

    // Clean-ish state
    VBO::unbind(GL_ARRAY_BUFFER);
    VAO::unbind();

    return q;
}
