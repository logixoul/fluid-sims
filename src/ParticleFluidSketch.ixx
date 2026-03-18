module;

#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <glad/glad.h>
#include <imgui.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/io.hpp>

#define MULTILINE(...) #__VA_ARGS__

export module ParticleFluidSketch;

import lxTextureRef;
import shade;
import stuff;
import gpgpu;
import gpuBlur2_5;
import Array2D_imageProc;
import IntegratedConsole;
import util;

using namespace std;
using namespace glm;

// windowSize, keys, mouseDown_ are defined in SketchScaffold module
extern glm::ivec2 windowSize;
extern bool keys[256];
extern bool mouseDown_[3];

export struct ParticleFluidSketch {
    typedef Array2D<float> Image;
    const int scale = 2;
    ivec2 sz;

    struct Particle {
        vec2 pos;
        vec2 velocity;
        vec2 force;
        float densityHere;
        float pressureHere;
        vec2 velocityContribs;
        float velocityContribsSumWeights;
    };

    vector<Particle> particles;

    void reset() {
        particles.clear();
    }

    bool pause = false;

    void setup()
    {
        sz = ivec2(::windowSize.x / scale, ::windowSize.y / scale);
        reset();
    }
    void keyDown(int key)
    {
        if (keys['r'])
        {
            reset();
        }
        if (keys['p'] || keys['2'])
        {
            pause = !pause;
        }
    }
    void keyUp(int key) {
    }
    vec2 direction;
    vec2 lastm;
    void mouseDrag(ivec2 pos)
    {
        mm_mouse(pos);
    }
    void mouseMove(ivec2 pos)
    {
        mm_mouse(pos);
    }
    void mm_mouse(ivec2 pos)
    {
        direction = vec2(pos) - lastm;
        lastm = pos;
    }
    float surfaceTensionThreshold = 1.0f;
    float pushawayCoef = .2f;
    float surfaceTensionCoef = 0.2f;
    void stefanDraw()
    {
        static float blurSize = 1.41f;
        ImGui::DragFloat("blurSize", &blurSize, 1.0f, 0.1, 100, "%.3f", ImGuiSliderFlags_Logarithmic);
        static int blurIters = 3;
        ImGui::DragInt("blurIters", &blurIters, 1.0f, 1, 16, "%d", ImGuiSliderFlags_None);
        static float gradScale = 1.68f;
        ImGui::DragFloat("gradScale", &gradScale, 1.0f, 1, 2000, "%.3f", ImGuiSliderFlags_Logarithmic);
        static float renderThreshold = 0.07f;
        ImGui::DragFloat("renderThreshold", &renderThreshold, 1.0f, .001f, 20, "%.3f", ImGuiSliderFlags_Logarithmic);

        ImGui::DragFloat("surfaceTensionThreshold", &surfaceTensionThreshold, .1f, .001f, 20.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::DragFloat("pushawayCoef", &pushawayCoef, .1f, .001f, 20.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::DragFloat("surfaceTensionCoef", &surfaceTensionCoef, .1f, .001f, 20.0f, "%.3f", ImGuiSliderFlags_Logarithmic);

        static float bloomSize = 1.5f;
        static int bloomIters = 6;
        static float bloomIntensity = 0.07f;
        ImGui::DragFloat("bloomSize", &bloomSize, 1.0f, 0.1, 100, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::DragInt("bloomIters", &bloomIters, 1.0f, 1, 16, "%d", ImGuiSliderFlags_None);
        ImGui::DragFloat("bloomIntensity", &bloomIntensity, 1.0f, 0.0001, 2000, "%.3f", ImGuiSliderFlags_Logarithmic);

        lxClear();

        auto img = Array2D<float>(sz);
        for (auto& particle : particles) {
            aaPoint<float, WrapModes::GetClamped>(img, particle.pos, 47.0f);
        }

        auto tex = gtex(img);
        auto texB = gpuBlur2_5::run(tex, blurIters);
        auto thresholdedTex = shade2(texB,
            "float f = fetch1();"
            "float fw = fwidth(f);"
            "f = smoothstep(0.15-fw/2, 0.15+fw/2, f);"
            "_out.r = f;");

        auto limitedTex = shade2(texB,
            "float f = fetch1();"
            "float fw = fwidth(f);"
            "f *= smoothstep(renderThreshold-fw/2, renderThreshold+fw/2, f);"
            "_out.r = f;",
            ShadeOpts().uniform("renderThreshold", renderThreshold)
        );

        auto redTex = thresholdedTex;

        auto redTexB = gpuBlur2_5::run_longtail(redTex, bloomIters, bloomSize);

        redTex = shade2(redTex, redTexB, MULTILINE(
            _out.r = (fetch1() + fetch1(tex2)) * bloomIntensity;
        ),
            ShadeOpts().uniform("bloomIntensity", bloomIntensity)
        );

        static const auto format = gl::Texture::Format().mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR).magFilter(GL_LINEAR).loadTopDown(true).wrap(GL_MIRRORED_REPEAT);
        static auto envMap = gl::Texture::create("milkyway.png", format);
        auto grads = get_gradients_tex(limitedTex);

        auto tex2 = shade2(texB, grads, envMap, redTex,
            "vec2 grad = fetch2(tex2) * gradScale;"
            "vec3 N = normalize(vec3(-grad.x, -grad.y, -1.0));"
            "vec3 I=-normalize(vec3(tc.x-.5, tc.y-.5, 0.2));"
            "float eta=1.0/1.3;"
            "vec3 R=refract(I, N, eta);"
            "vec3 c = getEnv(R);"
            "c*=0.5;\n"
            "vec3 albedo = 0.0*vec3(0.005, 0.0, 0.0);"
            "R = reflect(I, N);"
            "if(fetch1(tex) > surfTensionThres) {"
            "	vec3 refl = R;"
            "	vec3 normal = N;"
            "	vec3 viewDir = -I;"
            "	float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 5.0);"
            "	float fresnelWeight = mix(0.1, 1.0, fresnel);"
            "	c += getEnv(refl) * fresnelWeight;"
            "}"
            "float redVal = fetch1(tex4);"
            "redVal = 1.0-exp(-redVal);"
            "vec3 redColor = vec3(min(redVal * 1.5, 1.), pow(redVal, 2.5), pow(redVal, 12.)); "
            "c += redColor;"
            "_out.rgb = c;"
            , ShadeOpts().ifmt(GL_RGB16F).scale(4.0f).uniform("surfTensionThres", 0.1f).uniform("gradScale", gradScale),
            "float PI = 3.14159265358979323846264;\n"
            "vec2 latlong(vec3 refl) {\n"
            "	return vec2(atan(refl.z, refl.x) / (2.0 * PI) + 0.5, asin(clamp(refl.y, -1.0, 1.0)) / PI + 0.5);"
            "}\n"
            "vec3 getEnv(vec3 v) {\n"
            "	vec3 c = fetch3(tex3, latlong(v));\n"
            "	c = pow(c, vec3(2.2));"
            "	return c;"
            "}\n"
        );

        tex2 = shade2(tex2,
            "vec3 c = fetch3(tex);"
            "if(c.r<0.0||c.g<0.0||c.b<0.0) { _out.rgb = vec3(1.0, 0.0, 0.0); }"
            "c = pow(c, vec3(1.0/2.2));"
            "_out.rgb = c;"
        );

        lxDraw(tex);
    }

    int elapsedFrames = 0;
    void stefanUpdate()
    {
        if (!pause)
        {
            doFluidStep();
        }
        vec2 mousePos = this->lastm;
        mousePos /= scale;
        if (mouseDown_[0])
        {
            static float t = 0.0f;
            for (int i = 0; i < 5; i++) {
                Particle part; part.pos = mousePos + vec2(sin(t), cos(t)) * 30.0f;
                particles.push_back(part);
                t++;
            }
        }
        else if (mouseDown_[1]) {
            for (Particle& part : particles) {
                if (distance(part.pos, mousePos) < 40) {
                    const float velocityScaleFactor = 0.6f / (float)scale;
                    part.velocity += velocityScaleFactor * direction;
                    cout << velocityScaleFactor * direction << endl;
                    float speed = glm::length(part.velocity);
                    float newSpeed = std::min(speed, velocityScaleFactor * 30.0f);
                    part.velocity = part.velocity * newSpeed / speed;
                }
            }
        }
        elapsedFrames++;
    }

    float steepKernel(float dist, float radius) {
        if (dist >= radius) return 0.0f;
        float x = 1.0f - dist / radius;
        return x * x;
    }

    float smoothKernel(float dist, float radius) {
        if (dist >= radius) return 0.0f;
        return glm::smoothstep(1.0f, 0.0f, dist / radius);
    }

    const float MAX_DIST = 20;
    void forEachNeighbourPair(std::function<void(Particle&, Particle&, vec2 const&, float)> const& cb) {
        for (int i = 0; i < (int)particles.size(); i++) {
            auto& p1 = particles[i];
            for (int j = i + 1; j < (int)particles.size(); j++) {
                auto& p2 = particles[j];
                auto vec = p1.pos - p2.pos;
                float dist = length(vec);
                if (dist <= MAX_DIST) {
                    cb(p1, p2, vec, dist);
                }
            }
        }
    }

    void lxComputeForces() {
        for (auto& p : particles) {
            p.densityHere = 0;
            p.pressureHere = 0;
        }

        forEachNeighbourPair([&](auto& p1, auto& p2, vec2 const& vec, float dist) {
            float f = steepKernel(dist, MAX_DIST);
            float densityToAdd = f;
            p1.densityHere += densityToAdd;
            p2.densityHere += densityToAdd;
            });

        for (auto& p : particles) {
            p.pressureHere = p.densityHere - surfaceTensionThreshold;
        }

        forEachNeighbourPair([&](auto& p1, auto& p2, vec2 const& vec, float dist) {
            auto vecNorm = vec / (dist + 0.0001f);
            float f = steepKernel(dist, MAX_DIST);
            vec2 pushawayVec = vecNorm * f;
            const float pressureSum = p1.pressureHere + p2.pressureHere;
            p1.force += pushawayVec * surfaceTensionCoef * pressureSum;
            p2.force -= pushawayVec * surfaceTensionCoef * pressureSum;
            });

        smoothenVelocities();
    }

    void smoothenVelocities() {
        for (auto& p : particles) {
            float f = smoothKernel(0.0f, MAX_DIST);
            p.velocityContribs = p.velocity * f;
            p.velocityContribsSumWeights = f;
        }
        forEachNeighbourPair([&](Particle& p1, Particle& p2, vec2 const& vec, float dist) {
            float f = smoothKernel(dist, MAX_DIST);
            p1.velocityContribs += p2.velocity * f;
            p2.velocityContribs += p1.velocity * f;
            p1.velocityContribsSumWeights += f;
            p2.velocityContribsSumWeights += f;
            });
        for (auto& p : particles) {
            p.velocity = p.velocityContribs / p.velocityContribsSumWeights;
        }
    }

    void doFluidStep() {
        const auto maxX = sz.x;
        const auto maxY = sz.y;
        lxComputeForces();
        const float dampening = 0.75f;
        for (auto& p : particles) {
            if (p.pos.x < 0 || p.pos.x > maxX) {
                p.velocity.x *= -1;
                p.velocity.x *= dampening;
            }
            if (p.pos.y < 0 || p.pos.y > maxY) {
                p.velocity.y *= -1;
                p.velocity.y *= dampening;
            }
            if (p.pos.x < 0) {
                p.pos.x = -p.pos.x;
            }
            if (p.pos.x > maxX) {
                p.pos.x = (float)maxX - (p.pos.x - (float)maxX);
            }
            if (p.pos.y < 0) {
                p.pos.y = -p.pos.y;
            }
            if (p.pos.y > maxY) {
                p.pos.y = (float)maxY - (p.pos.y - (float)maxY);
            }
        }
        for (auto& p : particles) {
            p.velocity += p.force + vec2(0, 0.07f);
            p.pos += p.velocity;
            p.force = vec2(0, 0);
        }
    }
};
