#pragma once

#include "precompiled.h"
//#include "using_namespace.h"
#include "stuff.h"
#include "shade.h"
#include "gpgpu.h"
#include "gpuBlur2_5.h"
#include "Array2D_imageProc.h"
#include "util.h"
#include "SketchScaffold.h"
#include "lxAreaRectf.h"

#define GET_FLOAT_LOGSCALE(name, defaultValue, min, max) \
	static float name = defaultValue; \
	ImGui::DragFloat(#name, &name, 0.5f, min, max, "%.3f", ImGuiSliderFlags_Logarithmic);

struct MultiscaleGrayScottSketch {
	const int scale = 6;
	int sx;
	int sy;
	ivec2 sz;
	Array2D<vec2> grayScottState;
	
	void setup()
	{
		sx = ::windowSize.x / scale;
		sy = ::windowSize.y / scale;
		sz = ivec2(sx, sy);

		grayScottState = Array2D<vec2>(sz);

		glDisable(GL_BLEND);

		reset();
	}
	void keyDown()
	{
		if (keys['r'])
		{
			reset();
		}
	}
	void reset() {
		std::fill(grayScottState.begin(), grayScottState.end(), vec2(0.0f, 1.0f));

	}
	vec2 direction;
	vec2 lastm;
	void mouseMove(ivec2 pos)
	{
		mm(pos);
	}
	void mm(ivec2 pos)
	{
		direction = vec2(pos) - lastm;
		lastm = pos;
	}
	gl::TextureRef gtexF32(Array2D<float> a)
	{
		gl::TextureRef tex = maketex(a.w, a.h, GL_R32F);
		bind(tex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RED, GL_FLOAT, a.data);
		return tex;
	}

	gl::TextureRef gauss3texScaled(gl::TextureRef src, float scale) {
		auto state = shade2(src,
			"vec3 sum = vec3(0.0);"
			"sum += fetch3(tex, tc + tsize * vec2(-1.0, -1.0)) / 16.0;"
			"sum += fetch3(tex, tc + tsize * vec2(-1.0, 0.0)) / 8.0;"
			"sum += fetch3(tex, tc + tsize * vec2(-1.0, +1.0)) / 16.0;"

			"sum += fetch3(tex, tc + tsize * vec2(0.0, -1.0)) / 8.0;"
			"sum += fetch3(tex, tc + tsize * vec2(0.0, 0.0)) / 4.0;"
			"sum += fetch3(tex, tc + tsize * vec2(0.0, +1.0)) / 8.0;"

			"sum += fetch3(tex, tc + tsize * vec2(+1.0, -1.0)) / 16.0;"
			"sum += fetch3(tex, tc + tsize * vec2(+1.0, 0.0)) / 8.0;"
			"sum += fetch3(tex, tc + tsize * vec2(+1.0, +1.0)) / 16.0;"
			"_out.rgb = sum;",
			ShadeOpts().scale(scale)
		);
		return state;
	}

	void stefanDraw()
	{
		lxClear();

		static float colorAmount = 0.06f;
		ImGui::DragFloat("colorAmount", &colorAmount, 1.0f, 0.01, 8.0, "%.3f", ImGuiSliderFlags_Logarithmic);
		
		auto tex = gtex(grayScottState);
		lxDraw(tex);
	}
	void stefanUpdate()
	{
		doGrayScott();
		
		handleMouseInput();
	}

	void doGrayScott() {
		auto stateTex = gtex(grayScottState);
		auto stateTexLaplace = get_laplace_tex(stateTex, GL_REPEAT);
		stateTex = shade2(stateTex, stateTexLaplace, MULTILINE(
			vec2 state = texture(tex, tc).xy;
			vec2 stateLaplace = texture(tex2, tc).xy;
			float u = state.x;
			float v = state.y;
			float du = 0.16 * (stateLaplace.x - u) - u * v * v + 0.035 * (1.0 - u);
			float dv = 0.08 * (stateLaplace.y - v) + u * v * v - (0.06 + 0.035) * v;
			state.x += du;
			state.y += dv;
			_out.rg = state;
			), ShadeOpts().ifmt(GL_RG16F));
		//stateTex.read(grayScottState);
		grayScottState = dl<vec2>(stateTex);
	}

	void handleMouseInput() {
		vec2 mousePos = this->lastm;
		mousePos /= scale;
		if (mouseDown_[0])
		{

			lxArea a = lxArea(ivec2(mousePos), ivec2(mousePos));
			int r = 640 / pow(2, scale);
			a.expand(r, r);
			for (int x = a.x1; x <= a.x2; x++)
			{
				for (int y = a.y1; y <= a.y2; y++)
				{
					vec2 v = vec2(x, y) - mousePos;
					float w = std::max(0.0f, 1.0f - length(v) / r);
					w = 3 * w * w - 2 * w * w * w;
					auto& here = grayScottState.wr(x, y);
					here.x = mix(here.x, 1.0f, w * randFloat());
					here.y = mix(here.y, 1.0f, w * randFloat());
				}
			}
		}
	}

	template<class T, class FetchFunc>
	static Array2D<T> convolve(Array2D<T> in, Array2D<float> kernel) {
		int r = kernel.w / 2;
		auto out = ::empty_like(in);
		forxy(out) {
			float sum = 0.0f;
			for (int kx = -r; kx < r; kx++) {
				for (int ky = -r; ky < r; ky++) {
					sum += kernel(kx + r, ky + r) * FetchFunc::template fetch<T>(in, p.x + kx, p.y + ky);
				}
			}
			out(p) = sum;
		}
		return out;
	}
};
