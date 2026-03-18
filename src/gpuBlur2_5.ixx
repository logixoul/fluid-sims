module;

#include <cmath>
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <climits>
#include <algorithm>
#include <glad/glad.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

export module gpuBlur2_5;

import lxTextureRef;
import shade;
import stuff;
import gpgpu;

using namespace std;
using namespace glm;

export namespace gpuBlur2_5 {

gl::TextureRef run(gl::TextureRef src, int lvls);
gl::TextureRef run_longtail(gl::TextureRef src, int lvls, float lvlmul, float hscale = .5f, float vscale = .5f);
float getGaussW();
float gauss(float f, float width);
gl::TextureRef upscale(gl::TextureRef src, ivec2 toSize);
gl::TextureRef upscale(gl::TextureRef src, float hscale, float vscale);
gl::TextureRef singleblur(gl::TextureRef src, float hscale, float vscale, GLenum wrap = GL_CLAMP_TO_BORDER);

std::vector<gl::TextureRef> buildGaussianPyramid(gl::TextureRef src, int levels = INT_MAX);

} // namespace gpuBlur2_5

export namespace gpuBlur = gpuBlur2_5;

// ---- Implementation ----

namespace gpuBlur2_5 {

gl::TextureRef run(gl::TextureRef src, int lvls) {
    auto state = shade2(src, "_out.rgb = fetch3();");

    for (int i = 0; i < lvls; i++) {
        state = singleblur(state, .5, .5);
    }
    state = upscale(state, src->getSize());
    return state;
}

float logAB(float a, float b) {
    return std::log(b) / std::log(a);
}

gl::TextureRef run_longtail(gl::TextureRef src, int lvls, float lvlmul, float hscale, float vscale) {
    vector<float> weights;
    float sumw = 0.0f;
    int maxHLvls = (int)std::floor(logAB(1.0f / hscale, (float)src->getWidth()));
    int maxVLvls = (int)std::floor(logAB(1.0f / vscale, (float)src->getHeight()));
    int maxLvls = std::min(maxHLvls, maxVLvls);
    lvls = std::min(lvls, maxLvls);
    for (int i = 0; i < lvls; i++) {
        float w = std::pow(lvlmul, float(i));
        w += 1.0f;
        weights.push_back(w);
        sumw += w;
    }
    for (float& w : weights) {
        w /= sumw;
    }
    vector<gl::TextureRef> zoomstates;
    zoomstates.push_back(src);
    zoomstates[0] = shade2(zoomstates[0],
        "_out.rgb = fetch3() * _mul;",
        ShadeOpts().uniform("_mul", 1.0f / sumw));
    for (int i = 0; i < lvls; i++) {
        auto newZoomstate = singleblur(zoomstates[i], hscale, vscale);
        zoomstates.push_back(newZoomstate);
        if (newZoomstate->getWidth() < 1 || newZoomstate->getHeight() < 1) throw runtime_error("too many blur levels");
    }
    for (int i = lvls - 1; i > 0; i--) {
        auto upscaled = upscale(zoomstates[i], zoomstates[i - 1]->getSize());
        float w = std::pow(lvlmul, float(i));
        zoomstates[i - 1] = shade2(zoomstates[i - 1], upscaled,
            "vec3 acc = fetch3(tex);"
            "vec3 nextzoom = fetch3(tex2);"
            "vec3 c = acc + nextzoom * _mul;"
            "_out.rgb = c;"
            , ShadeOpts().uniform("_mul", w)
        );
    }
    return zoomstates[0];
}

float getGaussW() {
    return 0.75f;
}

float gauss(float f, float width) {
    return std::exp(-f * f / (width*width));
}

gl::TextureRef upscale(gl::TextureRef src, ivec2 toSize) {
    return upscale(src, float(toSize.x) / src->getWidth(), float(toSize.y) / src->getHeight());
}

gl::TextureRef upscale(gl::TextureRef src, float hscale, float vscale) {
    string lib =
        "float gauss(float f, float width) {"
        "	return exp(-f*f/(width*width));"
        "}"
        ;
    string shader =
        "	float gaussW = 0.75f;"
        "	vec2 offset = vec2(GB2_offsetX, GB2_offsetY);"
        "	vec2 tc2 = floor(tc * texSize) / texSize;"
        "	tc2 += tsize / 2.0;"
        "	vec2 frXY = (tc - tc2) * texSize;"
        "	float fr = (GB2_offsetX == 1.0) ? frXY.x : frXY.y;"
        "	vec3 aM1 = fetch3(tex, tc2 + (-1.0) * offset * tsize);"
        "	vec3 a0 = fetch3(tex, tc2 + (0.0) * offset * tsize);"
        "	vec3 aP1 = fetch3(tex, tc2 + (+1.0) * offset * tsize);"
        "	"
        "	float wM1=gauss(-1.0-fr, gaussW);"
        "	float w0=gauss(-fr, gaussW);"
        "	float wP1=gauss(1.0-fr, gaussW);"
        "	float sum=wM1+w0+wP1;"
        "	wM1/=sum;"
        "	w0/=sum;"
        "	wP1/=sum;"
        "	_out.rgb = wM1*aM1 + w0*a0 + wP1*aP1;";
    setWrapBlack(src);
    auto hscaled = shade2(src, shader,
        ShadeOpts()
            .scale(hscale, 1.0f)
            .uniform("GB2_offsetX", 1.0f)
            .uniform("GB2_offsetY", 0.0f)
        , lib);
    setWrapBlack(hscaled);
    auto vscaled = shade2(hscaled, shader,
        ShadeOpts()
            .scale(1.0f, vscale)
            .uniform("GB2_offsetX", 0.0f)
            .uniform("GB2_offsetY", 1.0f)
        , lib);
    return vscaled;
}

gl::TextureRef singleblur(gl::TextureRef src, float hscale, float vscale, GLenum wrap) {
    // GPU_SCOPE expanded inline:
    GpuScope _gpuScope_singleblur("singleblur");
    float gaussW = getGaussW();
    float w0 = gauss(0.0, gaussW);
    float w1 = gauss(1.0, gaussW);
    float w2 = gauss(2.0, gaussW);
    float sum = 2.0f*(w1+w2) + w0;
    w2 /= sum;
    w1 /= sum;
    w0 /= sum;
    std::stringstream weights;
    weights << std::fixed << "float w0=" << w0 << ", w1=" << w1 << ", w2=" << w2 << ";" << std::endl;
    string shader =
        "vec2 offset = vec2(GB2_offsetX, GB2_offsetY);"
        "vec3 aM2 = fetch3(tex, tc + (-2.0) * offset * tsize);"
        "vec3 aM1 = fetch3(tex, tc + (-1.0) * offset * tsize);"
        "vec3 a0 = fetch3(tex, tc + (0.0) * offset * tsize);"
        "vec3 aP1 = fetch3(tex, tc + (+1.0) * offset * tsize);"
        "vec3 aP2 = fetch3(tex, tc + (+2.0) * offset * tsize);"
        ""
        + weights.str() +
        "_out.rgb = w2 * (aM2 + aP2) + w1 * (aM1 + aP1) + w0 * a0;";

    setWrap(src, wrap);
    auto hscaled = shade2(src, shader,
        ShadeOpts()
            .scale(hscale, 1.0f)
            .uniform("GB2_offsetX", 1.0f)
            .uniform("GB2_offsetY", 0.0f)
        );
    setWrap(hscaled, wrap);
    auto vscaled = shade2(hscaled, shader,
        ShadeOpts()
        .uniform("GB2_offsetX", 0.0f)
        .uniform("GB2_offsetY", 1.0f)
        .scale(1.0f, vscale));
    return vscaled;
}

std::vector<gl::TextureRef> buildGaussianPyramid(gl::TextureRef src, int levels) {
    std::vector<gl::TextureRef> pyr;
    if (!src || levels <= 0) return pyr;

    src = shade2(src, "_out = texture(tex, tc);");
    pyr.push_back(src);
    auto cur = src;

    for (int i = 1; i < levels; ++i) {
        if (cur->getWidth() <= 1 || cur->getHeight() <= 1) break;

        auto next = singleblur(cur, 0.5f, 0.5f, GL_REPEAT);
        if (!next) break;
        if (next->getWidth() == cur->getWidth() && next->getHeight() == cur->getHeight()) break;

        pyr.push_back(next);
        cur = next;
    }

    return pyr;
}

} // namespace gpuBlur2_5
