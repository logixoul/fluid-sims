#include "precompiled.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

import lxlib.SketchScaffold;

import FftRaysSketch;

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow
) {
	StartupSketch sketch;
	SketchScaffold sketchScaffold(&sketch);
	sketchScaffold.setup();
	sketchScaffold.mainLoop();
}
