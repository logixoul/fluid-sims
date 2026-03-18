#include "precompiled.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

import lxlib.SketchScaffold;

import MultiscaleGrowthSketch;
import ParticleFluidSketch;
import ParticleTraces2DSketch;
import GridFluidSketch;

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
