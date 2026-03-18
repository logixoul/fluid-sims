#include "precompiled.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
// these two are #defined by Windows.h and cause problems with std::min and std::max, so we undefine them
#undef min
#undef max

#include "GridFluidSketch/GridFluidSketch.h"
#include "ParticleTraces2DSketch/ParticleTraces2DSketch.h"

import SketchScaffold;

import MultiscaleGrowthSketch;
import ParticleFluidSketch;
import MultiscaleGrowthSketch;

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow
) {
	//ParticleFluidSketch sketch;
	//GridFluidSketch sketch;
	//ParticleTraces2DSketch sketch;

	MultiscaleGrowthSketch sketch;
	SketchScaffold sketchScaffold(&sketch);
	sketchScaffold.setup();
	sketchScaffold.mainLoop();
}
