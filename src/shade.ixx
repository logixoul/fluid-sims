module;

#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <mutex>
#include <functional>
#include <map>
#include <memory>
#include <glad/glad.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/io.hpp>

export module shade;

import lxGlslProg;
import lxTextureRef;
import lxVaoVbo;
import util;

using namespace std;
using namespace glm;

// Forward declarations for functions defined in stuff module
// (avoids circular dependency: shade ← stuff ← shade)
void bind(gl::TextureRef& tex);
void bindTexture(gl::TextureRef& tex);
void bindTexture(gl::TextureRef tex, GLenum textureUnit);
gl::TextureRef maketex(int w, int h, GLint ifmt, bool allocateMipmaps = false, bool clear = false);

// ---- Exported declarations ----

export struct GpuScope {
    GpuScope(string name);
    ~GpuScope();
};

export void beginRTT(gl::TextureRef fbotex);
export void endRTT();
export void drawRect();

export struct Str {
    string s;
    Str& operator<<(string s2) {
        s += s2 + "\n";
        return *this;
    }
    Str& operator<<(Str s2) {
        s += s2.s + "\n";
        return *this;
    }
    operator std::string() {
        return s;
    }
};

export struct Uniform {
    function<void(lxGlslProgRef)> setter;
    string shortDecl;
};

export template<class T>
struct optional {
    T val;
    bool exists;
    optional(T const& t) { val=t; exists=true; }
    optional() { exists=false; }
};

export template<class T> string typeToString();
export template<> inline string typeToString<float>() { return "float"; }
export template<> inline string typeToString<int>() { return "int"; }
export template<> inline string typeToString<ivec2>() { return "ivec2"; }
export template<> inline string typeToString<vec2>() { return "vec2"; }

export struct ShadeOpts
{
    ShadeOpts();
    ShadeOpts& ifmt(GLenum val) { _ifmt=val; return *this; }
    ShadeOpts& scale(float val) { _scaleX=val; _scaleY=val; return *this; }
    ShadeOpts& scale(float valX, float valY) { _scaleX=valX; _scaleY=valY; return *this; }
    ShadeOpts& scope(std::string name) { _scopeName = name; return *this; }
    ShadeOpts& targetTex(gl::TextureRef val) { _targetTexs = { val }; return *this; }
    ShadeOpts& targetTexs(vector<gl::TextureRef> val) { _targetTexs = val; return *this; }
    ShadeOpts& targetImg(gl::TextureRef val) { _targetImg = val; return *this; }
    ShadeOpts& dstPos(ivec2 val) { _dstPos = val; return *this; }
    ShadeOpts& dstRectSize(ivec2 val) { _dstRectSize = val; return *this; }
    ShadeOpts& enableResult(bool val) {
        _enableResult = val; return *this;
    }
    template<class T>
    ShadeOpts& uniform(string name, T val) {
        _uniforms.push_back(Uniform{
            [val, name](lxGlslProgRef prog) { prog->uniform(name, val); },
            typeToString<T>() + " " + name
            });
        return *this;
    }
    ShadeOpts& vshaderExtra(string val) {
        _vshaderExtra = val;
        return *this;
    }

    optional<GLenum> _ifmt;
    float _scaleX, _scaleY;
    std::string _scopeName;
    vector<gl::TextureRef> _targetTexs;
    gl::TextureRef _targetImg = nullptr;
    ivec2 _dstPos;
    ivec2 _dstRectSize = ivec2(0, 0);
    bool _enableResult = true;
    vector<Uniform> _uniforms;
    string _vshaderExtra;
};

export gl::TextureRef shade(vector<gl::TextureRef> const& texv, std::string const& fshader, ShadeOpts const& opts=ShadeOpts());
export inline gl::TextureRef shade(vector<gl::TextureRef> const& texv, std::string const& fshader, float resScale)
{
    return shade(texv, fshader, ShadeOpts().scale(resScale));
}

// ---- Implementation ----

bool fboBound = false;

void beginRTT(gl::TextureRef fbotex)
{
    static unsigned int fboid = 0;
    if(fboid == 0)
    {
        glGenFramebuffers(1, &fboid);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fboid);
    if (fbotex != nullptr)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbotex->getId(), 0);
    fboBound = true;
}

static void beginRTT_multi(vector<gl::TextureRef> fbotexs)
{
    if (fbotexs.size() != 1)
        throw runtime_error("not implemented");

    static unsigned int fboid = 0;
    if (fboid == 0)
    {
        glGenFramebuffers(1, &fboid);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fboid);
    int i = 0;
    for (auto& fbotex : fbotexs) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, fbotex->getId(), 0);
        i++;
    }
    fboBound = true;
}

void endRTT()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    fboBound = false;
}

void drawRect() {
    static std::shared_ptr<QuadGpu> quad = createQuadVAO_VBOs();
    quad->vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    lxVAO::unbind();
}

auto samplerSuffix = [&](int i) -> string {
    return (i == 0) ? "" : std::to_string(1 + i);
};

auto samplerName = [&](int i) -> string {
    return "tex" + samplerSuffix(i);
};

std::string getCompleteFshader(vector<gl::TextureRef> const& texv, vector<Uniform> const& uniforms, std::string const& fshader, string* uniformDeclarationsRet) {
    stringstream uniformDeclarations;
    uniformDeclarations << "uniform ivec2 viewportSize;\n";
    uniformDeclarations << "uniform vec2 mouse;\n";
    for(int i = 0; i < (int)texv.size(); i++)
    {
        string samplerType = "sampler2D";
        uniformDeclarations << "uniform " + samplerType + " " + samplerName(i) + ";\n";
        uniformDeclarations << "uniform vec2 " + samplerName(i) + "Size;\n";
        uniformDeclarations << "uniform vec2 tsize" + samplerSuffix(i) + ";\n";
    }
    for (auto& p : uniforms)
    {
        uniformDeclarations << "uniform " + p.shortDecl + ";\n";
    }
    *uniformDeclarationsRet = uniformDeclarations.str();
    string intro =
        Str()
        << "#version 150"
        << "#extension GL_ARB_explicit_uniform_location : enable"
        << "#extension GL_ARB_texture_gather : enable"
        << uniformDeclarations.str()
        << "in vec2 tc;"
        << "in highp vec2 relOutTc;"
        << "/*precise*/ out vec4 _out;"
        ;
    intro += Str()
        << "vec4 fetch4(sampler2D tex_, vec2 tc_) {"
        << "	return texture2D(tex_, tc_).rgba;"
        << "}"
        << "vec3 fetch3(sampler2D tex_, vec2 tc_) {"
        << "	return texture2D(tex_, tc_).rgb;"
        << "}"
        << "vec2 fetch2(sampler2D tex_, vec2 tc_) {"
        << "	return texture2D(tex_, tc_).rg;"
        << "}"
        << "float fetch1(sampler2D tex_, vec2 tc_) {"
        << "	return texture2D(tex_, tc_).r;"
        << "}"
        << "vec4 fetch4(sampler2D tex_) {"
        << "	return texture2D(tex_, tc).rgba;"
        << "}"
        << "vec3 fetch3(sampler2D tex_) {"
        << "	return texture2D(tex_, tc).rgb;"
        << "}"
        << "vec2 fetch2(sampler2D tex_) {"
        << "	return texture2D(tex_, tc).rg;"
        << "}"
        << "float fetch1(sampler2D tex_) {"
        << "	return texture2D(tex_, tc).r;"
        << "}"
        << "vec4 fetch4() {"
        << "	return texture2D(tex, tc).rgba;"
        << "}"
        << "vec3 fetch3() {"
        << "	return texture2D(tex, tc).rgb;"
        << "}"
        << "vec2 fetch2() {"
        << "	return texture2D(tex, tc).rg;"
        << "}"
        << "float fetch1() {"
        << "	return texture2D(tex, tc).r;"
        << "}"
        << "vec2 safeNormalized(vec2 v) { return length(v)==0.0 ? v : normalize(v); }"
        << "vec3 safeNormalized(vec3 v) { return length(v)==0.0 ? v : normalize(v); }"
        << "vec4 safeNormalized(vec4 v) { return length(v)==0.0 ? v : normalize(v); }"
        << "#line 0\n\n"
        ;
    string outro =
        Str()
        << "void main()"
        << "{"
        << "	_out = vec4(0.0f, 0.0f, 0.0f, 1.0f);"
        << "	shade();"
        << "}";
    return intro + fshader + outro;
}

gl::TextureRef shade(vector<gl::TextureRef> const& texv, std::string const& fshader, ShadeOpts const& opts)
{
    shared_ptr<GpuScope> gpuScope;
    if (opts._scopeName != "") {
        gpuScope = make_shared<GpuScope>(opts._scopeName);
    }
    static std::mutex mapMutex;
    unique_lock<std::mutex> ul(mapMutex);
    static std::map<string, lxGlslProgRef> shaders;
    lxGlslProgRef shader;
    if(shaders.find(fshader) == shaders.end())
    {
        string uniformDeclarations;
        std::string completeFshader = getCompleteFshader(texv, opts._uniforms, fshader, &uniformDeclarations);
        string completeVshader = Str()
            << "#version 150"
            << "#extension GL_ARB_explicit_attrib_location : enable"
            << "layout(location = 0) in vec4 ciPosition;"
            << "layout(location = 1) in vec2 ciTexCoord0;"
            << "out highp vec2 tc;"
            << "out highp vec2 relOutTc;"
            << uniformDeclarations

            << "void main()"
            << "{"
            << "	gl_Position = ciPosition * 2 - 1;"
            << "	tc = ciTexCoord0;"
            << "	relOutTc = tc;"
            << opts._vshaderExtra
            << "}";
        try{
            shader = std::make_shared<lxGlslProg>(completeFshader, completeVshader);
            shaders[fshader] = shader;
        } catch(std::runtime_error const& e) {
            cout << "lxGlslProgCompileExc: " << e.what() << endl;
            cout << "source:" << endl;
            cout << completeFshader << endl;
            string s; cin >> s;
            throw;
        }
    } else {
        shader = shaders[fshader];
    }
    auto tex0 = texv[0];
    shader->bind();
    ivec2 viewportSize(
        (int)floor(tex0->getWidth() * opts._scaleX),
        (int)floor(tex0->getHeight() * opts._scaleY)
    );

    if (opts._dstRectSize != ivec2(0, 0)) {
        viewportSize = opts._dstRectSize;
    }
    vector<gl::TextureRef> results;
    if (opts._enableResult) {
        if (opts._targetTexs.size() != 0)
        {
            results = opts._targetTexs;
        }
        else {
            GLenum ifmt = opts._ifmt.exists ? opts._ifmt.val : tex0->getInternalFormat();
            results = { maketex(viewportSize.x, viewportSize.y, ifmt) };
        }
    }

    shader->uniform("viewportSize", viewportSize);
    shader->uniform("tex", 0); bindTexture(tex0, GL_TEXTURE0 + 0);
    shader->uniform("texSize", vec2(tex0->getSize()));
    shader->uniform("tsize", vec2(1.0f) / vec2(tex0->getSize()));
    for (int i = 1; i < (int)texv.size(); i++) {
        shader->uniform(samplerName(i), i); bindTexture(texv[i], GL_TEXTURE0 + i);
        shader->uniform(samplerName(i)+"Size", vec2(texv[i]->getSize()));
        shader->uniform("tsize" + samplerSuffix(i), vec2(1)/vec2(texv[i]->getSize()));
    }
    for (auto& uniform : opts._uniforms)
    {
        uniform.setter(shader);
    }

    shader->bind();

    if (opts._enableResult) {
        beginRTT_multi(results);
    }
    else {
        beginRTT(opts._targetImg);
        glColorMask(false, false, false, false);
    }

    {
        glViewport(0, 0, viewportSize.x, viewportSize.y);
        glDisable(GL_BLEND);
        ::drawRect();
    }
    if (opts._enableResult) {
        endRTT();
    }
    else {
        endRTT();
        glColorMask(true, true, true, true);
    }
    return results[0];
}

GpuScope::GpuScope(string name) {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
}

GpuScope::~GpuScope() {
    glPopDebugGroup();
}

ShadeOpts::ShadeOpts() {
    _scaleX = _scaleY = 1.0f;
}
