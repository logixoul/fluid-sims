module;
#include "precompiled.h"
#include <lxlib/simplexnoise.h>
#include <lxlib/macros.h>

import lxlib.Array2D;
import lxlib.Array2D_imageProc;
import lxlib.stuff;
import lxlib.TextureRef;
import lxlib.gpgpu;
import lxlib.gpuBlur2_5;
import lxlib.SketchBase;
import lxlib.shade;
import lxlib.colorspaces;
import lxlib.VaoVbo;
import lxlib.GlslProg;
import lxlib.KissFFTWrapper;

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
	const float saturation = 0.8f; // not 1.0 because we want tonemapping bright colors to desaturate them
	HsvF hsv(hue, saturation, lightness);
	return FromHSV(hsv);
}

struct Walker {
	glm::vec2 pos;
	glm::vec2 vel;
	float contribution;
};

export struct FftRaysSketch : public SketchBase {
	Array2D<FFT::Complex> freqDomainState;
	std::vector<Walker> walkers;
	const int scale = 4;
	
	void setup()
	{
		glm::ivec2 stateSize = windowSize / scale;
		freqDomainState = Array2D<FFT::Complex>(stateSize, nofill());
		reset();
	}

	void reset() {
        glm::ivec2 size = freqDomainState.Size();
		forxy(freqDomainState) {
           float wrappedX = std::min((float)p.x, (float)(size.x - p.x));
			float wrappedY = std::min((float)p.y, (float)(size.y - p.y));
			float distToOrigin = glm::length(glm::vec2(wrappedX, wrappedY));
			float amplitude = 1.0f / std::max(distToOrigin, 1.0f);
			amplitude *= 100.0f; // boost overall brightness
			float phase = randFloat(0.0f, 2.0f * (float)M_PI);
			freqDomainState(p) = FFT::Complex(
				amplitude * std::cos(phase),
				amplitude * std::sin(phase));
		}

		walkers = std::vector<Walker>(100);
		for(auto& walker : walkers) {
			walker.pos = glm::vec2(0.0f, 0.0f);//glm::vec2(randFloat(), randFloat()) * vec2(freqDomainState.Size());
			walker.contribution = randFloat() * 0.5f + 0.5f;
		}
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

	void update() {
		if (pause)
			return;

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
			aaPoint<FFT::Complex, WrapModes::GetWrapped>(freqDomainState, walker.pos, contribution);
		}
		freqDomainState(0, 0) = FFT::Complex(0, 0); // remove DC component to prevent it from dominating the image
	}

	void draw() {
		glClearColor(0, 0, 0.7, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, windowSize.x, windowSize.y);
		glDisable(GL_BLEND);

		auto spatialDomainState = FFT::inverseFftC2C(freqDomainState);
		Array2D<FFT::Complex> normalized = 0.25f * spatialDomainState / std::sqrt((float)(spatialDomainState.area));
		forxy(normalized) {
			const float len = std::abs(normalized(p));
			normalized(p) *= pow(len, 3.0f) / len;
		}

		Array2D<vec3> img(normalized.Size());
		forxy(img) {
			glm::vec2 vec2(normalized(p).real(), normalized(p).imag());
			img(p) = complexToColor_HSV(vec2);
		}
		auto tex = gtex(img);
		tex = shade2(tex, // upscale
			"_out.rgb = fetch4().rgb;"
			, ShadeOpts().dstRectSize(vec2(windowSize)));

		tex = shade2(tex,
			"vec2 localTc = tc - 0.5;"
			"vec3 col = vec3(0.0);"
			"const int NUM_STEPS = 10;"
			"for(int i = 0; i < NUM_STEPS; i++) {"
			"	col += fetch4(tex, localTc + 0.5).rgb * pow(.6, float(i));"
			"	localTc -= localTc * 0.1;" // "zoom blur" effect
			"}"
			"_out.rgb = col;");

		auto texb = gpuBlur2_5::run(tex, 4);
		tex = op(tex) + op(texb) * 1.0f;
		//tex = shade2(tex, "_out.rgb = fetch4().rgb * .1;");
		tex = shade2(tex,
			"_out.rgb = fetch4().rgb*1.0;"
			"_out.rgb /= _out.rgb + vec3(1.0);" // reinhard-ish tonemapping
		);
		lxDraw(tex);
	}
};

export using StartupSketch = FftRaysSketch;