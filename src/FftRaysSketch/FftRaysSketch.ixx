module;
#include "precompiled.h"
#include <lxlib/simplexnoise.h>
#include <lxlib/macros.h>

import lxlib.Array2D;
import lxlib.Array2D_imageProc;
import lxlib.stuff;
import lxlib.TextureRef;
import lxlib.gpgpu;
import lxlib.gpuBlur;
import lxlib.SketchBase;
import lxlib.shade;
import lxlib.colorspaces;
import lxlib.VaoVbo;
import lxlib.GlslProg;
import lxlib.KissFFTWrapper;
import lxlib.ConfigManager3;

using FFT = KissFFTWrapper<float>;

export module FftRaysSketch;

bool pause;
const double M_PI = 3.14159265359;
vec3 complexToColor_HSV(vec2 comp) {
	float hue = (float)M_PI + (float)atan2(comp.y, comp.x);
	hue /= (float)(2 * M_PI);
	float lightness = length(comp);
	//lightness = .5f;
	//lightness /= lightness + 1.0f;
	const float saturation = 0.9f; // not 1.0 because we want tonemapping bright colors to desaturate them
	HsvF hsv(hue, saturation, lightness);
	return FromHSV(hsv);
}

struct Walker {
	glm::vec2 pos;
	glm::vec2 vel;
};

export struct FftRaysSketch : public SketchBase {
	struct Options {
		float gain;
		float crepuscularFalloff;
		ConfigManager3 cfg;

		Options() : cfg("FftRaysConfig.toml") {}

		void init() {
			cfg.init();
		}

		void update() {
			gain = cfg.getFloat("gain");
			crepuscularFalloff = cfg.getFloat("crepuscularFalloff");
		}
	};

	Options options;

	Array2D<FFT::Complex> freqDomainState;
	Array2D<FFT::Complex> freqDomainStateNext;
	std::vector<Walker> walkers;
	const int scale = 4;
	
	void setup()
	{
		reset();

		options.init();
	}

	void reset() {
		freqDomainState = generateRandomState();
		freqDomainStateNext = generateRandomState();

		walkers = std::vector<Walker>(100);
		for(auto& walker : walkers) {
			walker.pos = glm::vec2(randFloat(), randFloat()) * vec2(freqDomainState.Size());
		}
	}

	Array2D<FFT::Complex> generateRandomState() {
		Array2D<FFT::Complex> state(windowSize / scale, nofill());
		forxy(state) {
			float wrappedX = std::min((float)p.x, (float)(state.w - p.x));
			float wrappedY = std::min((float)p.y, (float)(state.h - p.y));
			float distToOrigin = glm::length(glm::vec2(wrappedX, wrappedY));
			float amplitude = 1.0f / std::max(distToOrigin, 1.0f);
			amplitude *= 100.0f; // boost overall brightness
			float phase = randFloat(0.0f, 2.0f * (float)M_PI);
			state(p) = FFT::Complex(
				amplitude * std::cos(phase),
				amplitude * std::sin(phase));
		}
		return state;
	}

	void keyDown(int key)
	{
		if (key == 'p')
		{
			pause = !pause;
		}
		if (key == 'r')
		{
			reset();
		}
	}
	int elapsedFrames = 0;
	static const int NUM_FRAMES_BETWEEN_REGENERATIONS = 40;
	void update() {
		options.update();

		if (pause)
			return;
		elapsedFrames++;
		if(elapsedFrames % NUM_FRAMES_BETWEEN_REGENERATIONS == 0) {
			freqDomainState = freqDomainStateNext;
			freqDomainStateNext = generateRandomState();
		}


#if 0
		for(auto& walker : walkers) {
			float angle = randFloat() * 2 * M_PI;
			walker.vel = glm::vec2(cos(angle), sin(angle));

			walker.pos += walker.vel;
			/*if (walker.pos.x < 0 || walker.pos.x >= freqDomainState.w)
				walker.vel.x *= -1;
			if (walker.pos.y < 0 || walker.pos.y >= freqDomainState.h)
				walker.vel.y *= -1;*/


			FFT::Complex const contribution(randFloat(-1, 1) * 5.0f, randFloat(-1, 1) * 5.0f);
			//cout << "Adding contribution " << contribution << " at " << walker.pos << endl;
			splatBilinearPoint<FFT::Complex, WrapModes::Wrap>(freqDomainState, walker.pos, contribution);
		}
		freqDomainState(0, 0) = FFT::Complex(0, 0); // remove DC component to prevent it from dominating the image

#endif
	}

	FFT::Complex mix(FFT::Complex const& a, FFT::Complex const& b, float t) {
		return a * (1.0f - t) + b * t;
	}

	Array2D<FFT::Complex> darken(Array2D<FFT::Complex> const& in) {
		Array2D<FFT::Complex> result(in.Size(), nofill());
		forxy(result) {
			const float len = std::abs(in(p));
			const float newLen = std::pow(len, 3.0f);
			result(p) = in(p) * newLen / len;
		}
		return result;
	}

	void draw() {
		glClearColor(0, 0, 0.7, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, windowSize.x, windowSize.y);
		glDisable(GL_BLEND);

		auto spatialDomainState = FFT::inverseFftC2C(freqDomainState) / std::sqrt((float)(freqDomainState.area));
		spatialDomainState = darken(spatialDomainState);
		auto spatialDomainStateNext = FFT::inverseFftC2C(freqDomainStateNext) / std::sqrt((float)(freqDomainStateNext.area));
		spatialDomainStateNext = darken(spatialDomainStateNext);
		auto mixedState = uninitializedArrayLike(spatialDomainState);
		forxy(mixedState) {
			const float normalizedStateAge = (float)(elapsedFrames % NUM_FRAMES_BETWEEN_REGENERATIONS) / (float)NUM_FRAMES_BETWEEN_REGENERATIONS;
			float interpolationCoef = 1.0f - std::pow(1.0f - normalizedStateAge, 2.0f); // ease out curve
			mixedState(p) = mix(spatialDomainState(p), spatialDomainStateNext(p), interpolationCoef);
		}

		Array2D<vec3> img(mixedState.Size(), nofill());
		forxy(img) {
			glm::vec2 vec2(mixedState(p).real(), mixedState(p).imag());
			img(p) = complexToColor_HSV(vec2);
		}
		auto tex = uploadTex(img);
		tex = shade(tex, // upscale
			"_out = texture();"
			, ShadeOpts().dstRectSize(vec2(windowSize)));

		tex = shade(tex,
          "vec2 localTc = texCoord - 0.5;"
			"localTc *= 1.5; /* look from 'up high' */"
			"vec3 col = vec3(0.0);"
			"const int NUM_STEPS = 300;"
			"float sumWeights = 0.0f;"
			"for(int i = 0; i < NUM_STEPS; i++) {"
			"	float weight = pow(crepuscularFalloff, float(i));" // exponential weight falloff
			"	if(i == 0) weight *= 15.0f;" // boost center sample
           "	col += texture(tex0, localTc + 0.5).rgb * weight;"
			"	localTc -= localTc * 0.005;" // "zoom blur" effect
			"	sumWeights += weight;"
			"}"
			"_out.rgb = col / sumWeights;",
				ShadeOpts().uniform("crepuscularFalloff", options.crepuscularFalloff));

		auto texb = gpuBlur::run(tex, 3);
		tex = op(tex) + op(texb);
		//tex = shade(tex, "_out.rgb = texture().rgb * .1;");
		tex = shade(tex,
			"_out.rgb = texture().rgb*gain;"
			//"_out.rgb = reinhardTonemapLuma(_out.rgb);"
			//"_out.rgb = uncharted2Tonemap(_out.rgb);" // filmic tonemapping
			"_out.rgb = clipChroma(_out.rgb);" // chroma clipping to prevent colors from going out of gamut after tonemapping
			//"_out.rgb /= _out.rgb + vec3(1.0);" // reinhard-ish tonemapping
			//"_out.rgb = smoothstep(vec3(0.0), vec3(1.0), _out.rgb);" // contrast enhancement
			//"_out.rgb = pow(_out.rgb, vec3(1.0/2.2));" // gamma correction
			, ShadeOpts().uniform("gain", options.gain)
				.functions(R"(
					const float whitePoint = 11.2;
					// http://filmicworlds.com/blog/filmic-tonemapping-operators/
					vec3 uncharted2Tonemap(vec3 color) {
						// Filmic tonemapping curve (from Uncharted 2)
						const float A = 0.15;
						const float B = 0.50;
						const float C = 0.10;
						const float D = 0.20;
						const float E = 0.02;
						const float F = 0.30;
						return (color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F);
					}
					vec3 rgbWeights = vec3(0.22, 0.71, 0.07);
					vec3 reinhardTonemapLuma(vec3 color) {
						float luma = dot(color, rgbWeights);
						float tonemappedLuma = luma / (luma + 1.0);
						return color * (tonemappedLuma / luma);
					}
					vec3 clipChroma(vec3 inColor) {
						float luma = dot(inColor, rgbWeights);
						if(luma > 1.0) {
							return vec3(1.0);
						}
						vec3 gray = vec3(luma);

						vec3 chroma = inColor - gray;

						// If already inside cube, return as-is
						if (all(lessThanEqual(inColor, vec3(1.0)))) {
							return inColor;
						}

						float t = 1.0;

						for (int i = 0; i < 3; i++) {
							if (inColor[i] > 1.0) {
								t = min(t, (1.0 - gray[i]) / chroma[i]);
							}
						}
    
						return gray + chroma * t;
					}
				)")
		);
		lxDraw(tex);
	}
};

export using StartupSketch = FftRaysSketch;