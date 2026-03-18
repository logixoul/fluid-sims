#include "precompiled.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
import SketchScaffold;
import MultiscaleGrowthSketch;

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow
) {
	MultiscaleGrowthSketch sketch;
	SketchScaffold sketchScaffold(&sketch);
	sketchScaffold.setup();
	sketchScaffold.mainLoop();
}
