#pragma once
#include <lxlib/precompiled.h>
import lxlib.TextureRef;
#include <glm/vec2.hpp>
import lxlib.util;
using glm::ivec2;

namespace gpuBlurClaude {

	Array2D<float> singleblurLikeCinder(Array2D<float> src, ivec2 dstSize);
	gl::TextureRef singleblurLikeCinder(gl::TextureRef src, ivec2 dstSize);
	std::vector<gl::TextureRef> buildGaussianPyramid(gl::TextureRef const& src, float scalePerLevel = 0.5f);
	gl::TextureRef blurWithInvKernel(gl::TextureRef const& src);
}
