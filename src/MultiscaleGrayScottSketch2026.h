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

	// tunable Gray-Scott params (exposed in the UI)
	float Du = 0.2f;
	float Dv = 0.1f;
	float feed = 0.03f;
	float kill = 0.062f;
	int subSteps = 8; // smaller time-steps per frame stabilizes the sim
	float dt = 1.0f;  // per-substep time scale

	void setup()
	{
		sx = ::windowSize.x / scale;
		sy = ::windowSize.y / scale;
		sz = ivec2(sx, sy);

		grayScottState = Array2D<vec2>(sz);

		glDisable(GL_BLEND);

		reset();
	}
	void keyDown(char c)
	{
		if (c == 'r')
		{
			reset();
		}
	}
	void keyUp(char c)
	{

	}
	void reset() {
		// Base state: U ~ 1, V ~ 0 (standard Gray-Scott baseline)
		std::fill(grayScottState.begin(), grayScottState.end(), vec2(1.0f, 0.0f));
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

		ImGui::Text("Gray-Scott params");
		ImGui::DragFloat("Du", &Du, 0.001f, 0.0f, 2.0f, "%.5f");
		ImGui::DragFloat("Dv", &Dv, 0.001f, 0.0f, 2.0f, "%.5f");
		ImGui::DragFloat("feed (F)", &feed, 0.0005f, 0.0f, 1.0f, "%.5f");
		ImGui::DragFloat("kill (k)", &kill, 0.0005f, 0.0f, 1.0f, "%.5f");
		ImGui::DragInt("subSteps", &subSteps, 1, 1, 16);
		ImGui::DragFloat("dt (per step)", &dt, 0.001f, 0.001f, 1.0f, "%.4f");
		// clamp sanity
		if (subSteps < 1) subSteps = 1;
		if (dt <= 0.0f) dt = 1e-3f;

		static float colorAmount = 0.06f;
		ImGui::DragFloat("colorAmount", &colorAmount, 1.0f, 0.01, 8.0, "%.3f", ImGuiSliderFlags_Logarithmic);

		auto tex = gtex(grayScottState);
		lxDraw(tex);
	}
	static gl::TextureRef zerosLike(gl::TextureRef in) {
		//return gl::TextureRef(new lxTexture(in->getWidth(), in->getHeight(), lxTexture::Format().internalFormat(in->getInternalFormat()).wrap(in->getWrap)));
		return shade2(in, "_out = vec4(0.0);");
	}
	void stefanUpdate()
	{
		auto stateTex = gtex(grayScottState);

		auto pyr = gpuBlur2_5::buildGaussianPyramid(stateTex, 3);
		auto accumulatedChange = zerosLike(pyr[0]);
		int idx = 0;
		for (auto& lvl : pyr) {
			auto transformed = doGrayScott(lvl);
			auto diff = shade2(lvl, transformed, MULTILINE(
				vec2 state = texture(tex, tc).xy;
				vec2 newState = texture(tex2, tc).xy;
				_out.rg = newState - state;
			));
			// upsample and accumulate changes from this level
			diff->setWrap(GL_REPEAT);
			float const weight = pow(.5f, (float)idx);
			accumulatedChange = shade2(accumulatedChange, diff, MULTILINE(
				vec2 change = texture(tex, tc).xy;
				_out.rg = texture(tex2, tc).xy + change * weight; // add up changes from this level
					),
				ShadeOpts().uniform("weight", weight)
			);
			idx++;
		}
		stateTex = shade2(stateTex, accumulatedChange, MULTILINE(
			vec2 state = texture(tex, tc).xy;
			vec2 change = texture(tex2, tc).xy;
			_out.rg = state + change; // add up changes
			_out.rg = clamp(_out.rg, vec2(0.0), vec2(1.0)); // prevent runaway values
			),
			ShadeOpts().uniform("numLevels", float(pyr.size()))
			);
		grayScottState = dl<vec2>(stateTex);
		float minA = FLT_MAX, maxA = FLT_MIN, minB = FLT_MAX, maxB = FLT_MIN;
		/*for (auto& v : grayScottState) {
			minA = std::min(minA, v.x);
			maxA = std::max(maxA, v.x);
			minB = std::min(minB, v.y);
			maxB = std::max(maxB, v.y);
		}
		for (auto& v : grayScottState) {
			//v.x = (v.x - minA) / (maxA - minA);
			//v.y = (v.y - minB) / (maxB - minB);
			//v = glm::atan(v);
			//v = (v + vec2(::pi)) / (2.0f * ::pi);
		}*/

		handleMouseInput();
	}

	gl::TextureRef doGrayScott(gl::TextureRef in) {
		// do several sub-steps per frame with smaller dt to stabilize numerics
		auto stateTex = in;
		for (int s = 0; s < subSteps; ++s) {
			auto stateTexLaplace = get_laplace_tex(stateTex, GL_REPEAT);

			// build shader source with the current numerical parameters embedded.
			// using to_string is fine since the shader source is compiled immediately.
			std::string src;
			src += "vec2 state = texture(tex, tc).xy;\n";
			src += "vec2 stateLaplace = texture(tex2, tc).xy;\n";
			src += "float u = state.x;\n";
			src += "float v = state.y;\n";
			src += "float Du = " + std::to_string(Du) + ";\n";
			src += "float Dv = " + std::to_string(Dv) + ";\n";
			src += "float F = " + std::to_string(feed) + ";\n";
			src += "float k = " + std::to_string(kill) + ";\n";
			src += "float dt = " + std::to_string(dt) + ";\n";
			src += "float du = Du * stateLaplace.x - u * v * v + F * (1.0 - u);\n";
			src += "float dv = Dv * stateLaplace.y + u * v * v - (k + F) * v;\n";
			src += "state.x += dt * du;\n";
			src += "state.y += dt * dv;\n";
			src += "_out.rg = state;\n";

			stateTex = shade2(stateTex, stateTexLaplace, src.c_str(), ShadeOpts().ifmt(GL_RG16F));
		}
		return stateTex;
		// read back to CPU structure for mouse interaction / draw
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
