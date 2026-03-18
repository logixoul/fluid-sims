module;

#include <glad/glad.h>
#include <stdexcept>
#include <string>
#include <memory>
#include <cmath>
#include "stb_image.h"
#include <glm/vec2.hpp>

export module lxTextureRef;

import lxGlslProg;

// Forward declarations to avoid circular dependency with SketchScaffold
// (windowSize is defined in SketchScaffold, drawRect in shade)
extern glm::ivec2 windowSize;
extern void drawRect();

export class lxTexture
{
public:
    class Format {
    public:
        GLint mInternalFormat = -1;
        bool mImmutableStorage = false;
        bool mMipmapping = false;
        bool mLoadTopDown = false;
        GLenum mMinFilter, mMagFilter;
        GLenum mWrapS, mWrapT, mWrapR;
    public:
        void    setInternalFormat(GLint internalFormat) { mInternalFormat = internalFormat; }
        void    setImmutableStorage(bool immutable = true) { mImmutableStorage = immutable; }
        void    enableMipmapping(bool enableMipmapping = true) { mMipmapping = enableMipmapping; }
        Format& mipmap(bool enableMipmapping = true) { mMipmapping = enableMipmapping; return *this; }
        Format& minFilter(GLenum minFilter) { mMinFilter = minFilter; return *this; }
        Format& magFilter(GLenum magFilter) { mMagFilter = magFilter; return *this; }
        Format& loadTopDown(bool loadTopDown = true) { mLoadTopDown = loadTopDown; return *this; }
        Format& wrap(GLenum wrap) { mWrapS = mWrapT = mWrapR = wrap; return *this; }
        Format& internalFormat(GLenum internalFormat) { mInternalFormat = internalFormat; return *this; }
    };
private:
    GLuint mId;
    Format mFormat;
    int mWidth, mHeight;
    bool mTopDown = false;
public:
    lxTexture(int width, int height, Format const& format) {
        init(width, height, format);
    }
    lxTexture(std::string filePath, Format const& format) {
        int width, height, n;
        filePath = "../assets/" + filePath;
        unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &n, 4);
        if (data == nullptr)
            throw std::runtime_error("Couldn't load image");

        init(width, height, format);
        bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
        if (format.mMipmapping)
            glGenerateMipmap(GL_TEXTURE_2D);
    }
    ~lxTexture() {
        glDeleteTextures(1, &mId);
    }
    void bind() {
        glBindTexture(GL_TEXTURE_2D, mId);
    }
    GLint getId() {
        return mId;
    }
    GLenum getInternalFormat() const {
        return mFormat.mInternalFormat;
    }
    int getWidth() {
        return mWidth;
    }
    int getHeight() {
        return mHeight;
    }
    glm::ivec2 getSize() {
        return glm::ivec2(mWidth, mHeight);
    }
    void setTopDown(bool topDown) {
        mTopDown = topDown;
    }
    void setMinFilter(GLenum minFilter) {
        mFormat.minFilter(minFilter);
    }
    void setMagFilter(GLenum magFilter) {
        mFormat.magFilter(magFilter);
    }
    void setWrap(GLenum wrap) {
        mFormat.wrap(wrap);
    }
    void setWrap(GLenum wrapS, GLenum wrapT) {
        mFormat.mWrapS = wrapS;
        mFormat.mWrapT = wrapT;
    }
    GLenum getTarget() const {
        return GL_TEXTURE_2D;
    }
    static std::shared_ptr<lxTexture> create(int width, int height, Format const& format) {
        return std::make_shared<lxTexture>(width, height, format);
    }
    static std::shared_ptr<lxTexture> create(std::string filepath, Format const& format) {
        return std::make_shared<lxTexture>(filepath, format);
    }

private:
    void init(int width, int height, Format const& format) {
        mFormat = format;
        mWidth = width;
        mHeight = height;

        glGenTextures(1, &mId);
        bind();
        int levels;
        if (format.mMipmapping)
            levels = (int)std::floor(std::log2((float)std::max(width, height))) + 1;
        else
            levels = 1;
        glTexStorage2D(GL_TEXTURE_2D, levels, format.mInternalFormat, width, height);
    }
};

export typedef std::shared_ptr<lxTexture> lxTextureRef;

export namespace gl {
    using TextureRef = lxTextureRef;
    using Texture = lxTexture;
}

export inline const std::string genericVertexShaderSource =
"#version 150\n"
"in vec4 ciPosition;"
"in vec2 ciTexCoord0;"
"out highp vec2 tc;"
"uniform vec2 uTexCoordOffset, uTexCoordScale;"
"void main()"
"{"
"	gl_Position = ciPosition * 2 - 1;"
"	tc = ciTexCoord0;"
"	tc = uTexCoordOffset + uTexCoordScale * tc;"
"}";

export inline const std::string genericFragmentShaderSource =
"#version 150\n"
"in highp vec2 tc;"
"uniform sampler2D uTex;"
"void main()"
"{"
"	gl_FragColor = texture2D(uTex, tc);"
"}";

export inline void lxDraw(lxTextureRef const& tex) {
    glActiveTexture(GL_TEXTURE0);
    tex->bind();

    static const auto glsl = std::make_shared<lxGlslProg>(genericFragmentShaderSource, genericVertexShaderSource);
    glsl->bind();
    glsl->uniform("uTex", 0);
    glsl->uniform("uPositionOffset", glm::vec2(0, 0));
    glsl->uniform("uPositionScale", glm::vec2(tex->getSize()));
    glsl->uniform("uTexCoordOffset", glm::vec2(0.0f, 1.0f));
    glsl->uniform("uTexCoordScale", glm::vec2(1.0f, -1.0f));

    glViewport(0, 0, ::windowSize.x, ::windowSize.y);

    ::drawRect();
}

export inline void lxClear() {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}
