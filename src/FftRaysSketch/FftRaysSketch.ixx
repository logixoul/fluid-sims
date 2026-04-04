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

export module FftRaysSketch;

int wsx = 1280, wsy = 720;
int scale = 8;
int sx = wsx / ::scale;
int sy = wsy / ::scale;


bool pause;
const double M_PI = 3.14159265359;
vec3 complexToColor_HSV(vec2 comp) {
	float hue = (float)M_PI + (float)atan2(comp.y, comp.x);
	hue /= (float)(2 * M_PI);
	float lightness = length(comp);
	lightness = .5f;
	//lightness /= lightness + 1.0f;
	HslF hsl(hue, 1.0f, lightness);
	return FromHSL(hsl);
}

export struct FftRaysSketch : public SketchBase {
	Array2D<float> state;
	Array2D<float> get_variance(Array2D<float> const& in);

	void setup()
	{
		state = Array2D<float>(sx, sy);
		reset();
	}
	void reset() {
		forxy(state) {
			state(p) = randFloat();
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

		auto variance = get_variance(state);
		forxy(state) {
			state(p) += variance(p) * 0.1f;
		}
		state = to01(state);
	}

	void draw() {
		glClearColor(0, 0, 0.7, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, wsx, wsy);
		glDisable(GL_BLEND);
		auto tex = gtex(state);
		lxDraw(tex);
	}
};

Array2D<float> FftRaysSketch::get_variance(Array2D<float> const& in)
{
	Array2D<float> out(in.w, in.h, 0.0f);

	auto clamp_index = [](int v, int maxv) {
		if (v < 0) return 0;
		if (v > maxv) return maxv;
		return v;
	};

	for (int y = 0; y < in.h; ++y)
	{
		for (int x = 0; x < in.w; ++x)
		{
			float sum = 0.0f;
			float sumSq = 0.0f;

			for (int dy = -1; dy <= 1; ++dy)
			{
				int yy = clamp_index(y + dy, in.h - 1);
				for (int dx = -1; dx <= 1; ++dx)
				{
					int xx = clamp_index(x + dx, in.w - 1);
					float v = in(xx, yy);
					sum += v;
					sumSq += v * v;
				}
			}

			float mean = sum / 9.0f;
			out(x, y) = sumSq / 9.0f - mean * mean;
		}
	}

	return out;
}

export using StartupSketch = FftRaysSketch;