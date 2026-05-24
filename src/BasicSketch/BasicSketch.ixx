module;
#include "precompiled.h"

import lxlib.Array2D;
import lxlib.Array2D_imageProc;
import lxlib.stuff;
import lxlib.TextureRef;
import lxlib.gpgpu;
import lxlib.gpuBlur;
import lxlib.SketchBase;
import lxlib.shade;
import lxlib.VaoVbo;
import lxlib.GlslProg;

export module BasicSketch;

export struct BasicSketch : public lx::SketchBase {
	lx::Array2D<float> state;

	void setup()
	{
		state = lx::Array2D<float>(500, 500);
		reset();
	}
	void reset() {
      for(auto p : state.coords()) {
         state(p) = lx::randFloat();
		}
	}
	void keyDown(int key)
	{
		if (key == 'r')
		{
			reset();
		}
	}

	void update() {
		state = lx::gauss3(state);
		for (glm::ivec2 p : state.coords()) {
			state(p) = glm::smoothstep(0.0f, 1.0f, state(p));
		}
	}

	void draw() {
		glClearColor(0, 0, 0.7, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_BLEND);
        auto tex = lx::uploadTex(state);

		tex = lx::shade({tex}, R"(
			_out.rgb = vec3(lxTexture().r);
		)",
			lx::ShadeOpts().ifmt(GL_RGBA16F)
			);

		glViewport(0, 0, windowSize.x, windowSize.y);
		lx::lxDraw(tex, lx::Rect<float>(0, 0, 1, 1));
	}
};
