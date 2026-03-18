module;

#include <glad/glad.h>
#include <stdexcept>
#include <cstddef>
#include <cstdint>

export module lxVaoVbo;

export class lxVBO {
public:
    lxVBO() {
        glGenBuffers(1, &m_id);
        if (!m_id) throw std::runtime_error("glGenBuffers failed");
    }

    ~lxVBO() {
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

export class lxVAO {
public:
    lxVAO() {
        glGenVertexArrays(1, &m_id);
        if (!m_id) throw std::runtime_error("glGenVertexArrays failed");
    }

    ~lxVAO() {
        if (m_id) glDeleteVertexArrays(1, &m_id);
    }

    void bind() const { glBindVertexArray(m_id); }
    static void unbind() { glBindVertexArray(0); }

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

export struct QuadGpu {
    lxVAO vao;
    lxVBO vboPos;
    lxVBO vboUV;
};

export inline std::shared_ptr<QuadGpu> createQuadVAO_VBOs()
{
    static const float positions[6][4] = {
        {0.f, 0.f, 0.f, 1.f},
        {0.f, 1.f, 0.f, 1.f},
        {1.f, 1.f, 0.f, 1.f},
        {0.f, 0.f, 0.f, 1.f},
        {1.f, 1.f, 0.f, 1.f},
        {1.f, 0.f, 0.f, 1.f},
    };

    static const float texcoords[6][2] = {
        {0.f, 0.f},
        {0.f, 1.f},
        {1.f, 1.f},
        {0.f, 0.f},
        {1.f, 1.f},
        {1.f, 0.f},
    };

    auto q = std::make_shared<QuadGpu>();

    q->vao.bind();

    q->vboPos.setData(positions, sizeof(positions), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
    q->vboPos.bind(GL_ARRAY_BUFFER);
    q->vao.defineAttrib(0, 4, GL_FLOAT, GL_FALSE, 4 * (GLsizei)sizeof(float), (const void*)0);

    q->vboUV.setData(texcoords, sizeof(texcoords), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
    q->vboUV.bind(GL_ARRAY_BUFFER);
    q->vao.defineAttrib(1, 2, GL_FLOAT, GL_FALSE, 2 * (GLsizei)sizeof(float), (const void*)0);

    lxVBO::unbind(GL_ARRAY_BUFFER);
    lxVAO::unbind();

    return q;
}
