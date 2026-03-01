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
		// Base state: U ~ 1, V ~ 0 (standard Gray-Scott baseline)
		std::fill(grayScottState.begin(), grayScottState.end(), vec2(1.0f, 0.0f));

		// Add a handful of random "blobs" to seed pattern formation.
		// Keep perturbations moderate so they don't instantly blow up the simulation.
		const int numSeeds = 30;
		for (int i = 0; i < numSeeds; ++i) {
			int cx = int(randFloat() * grayScottState.w);
			int cy = int(randFloat() * grayScottState.h);
			float radius = 4.0f + randFloat() * 60.0f; // 4 .. 64
			float ampV = 0.25f + randFloat() * 0.6f;   // how much V we inject
			int x0 = std::max(0, int(cx - radius));
			int x1 = std::min(grayScottState.w - 1, int(cx + radius));
			int y0 = std::max(0, int(cy - radius));
			int y1 = std::min(grayScottState.h - 1, int(cy + radius));

			for (int x = x0; x <= x1; ++x) {
				for (int y = y0; y <= y1; ++y) {
					vec2 d = vec2(float(x - cx), float(y - cy));
					float nd = length(d) / radius;
					if (nd > 1.0f) continue;
					// smooth falloff
					float w = 1.0f - nd;
					w = w * w * (3.0f - 2.0f * w);

					auto& here = grayScottState(x, y); // use non-wrapping access
					// add V, reduce U slightly (kept small)
					here.y = std::min(1.0f, here.y + ampV * w);
					here.x = std::max(0.0f, here.x - (ampV * 0.8f) * w);
				}
			}
		}

		// Add low-amplitude random noise across the field to break symmetry.
		forxy(grayScottState) {
			auto& here = grayScottState(p);
			float n = (randFloat() - 0.5f) * 0.02f; // +/- 0.01
			here.y = glm::clamp(here.y + n, 0.0f, 1.0f);
		}
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
		// convert mouse to simulation coordinates
		vec2 mousePos = this->lastm;
		mousePos /= float(scale);

		if (!mouseDown_[0])
			return;

		// Controlled additive perturbation:
		// increase V slightly and decrease U slightly where the brush hits.
		// Use operator() (no wrap) so we don't inject across edges unexpectedly.
		// Keep changes small and clamp to [0,1] to avoid runaway values.
		lxArea a = lxArea(ivec2(mousePos), ivec2(mousePos));
		// radius scaled similarly to the original, but guarded
		int r = std::max(1, int(640.0f / powf(2.0f, float(scale))));
		a.expand(r, r);

		for (int x = a.x1; x <= a.x2; x++)
		{
			for (int y = a.y1; y <= a.y2; y++)
			{
				ivec2 ip(x, y);
				if (!grayScottState.contains(ip)) // avoid wrap-around artifacts
					continue;

				vec2 v = vec2(x, y) - mousePos;
				float w = std::max(0.0f, 1.0f - length(v) / float(r));
				// smootherstep-like falloff
				w = w * w * (3.0f - 2.0f * w);

				auto& here = grayScottState(ip); // no wrap
				// Add V and remove some U (kept small)
				float addV = 0.25f * w * (0.5f + 0.5f * randFloat()); // ~0..0.25 scaled by w
				float subU = 0.12f * w * (0.5f + 0.5f * randFloat());

				here.y = std::min(1.0f, here.y + addV);
				here.x = std::max(0.0f, here.x - subU);
			}
		}
	}
};
