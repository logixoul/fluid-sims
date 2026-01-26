#include "precompiled.h"
#include "ParticleFluidSketch.h"

struct SketchScaffold : ci::app::App {
	ParticleFluidSketch sketch;

	shared_ptr<IntegratedConsole> integratedConsole;
	void setup()
	{
		cfg2::init();
		integratedConsole = make_shared<IntegratedConsole>();;

		enableDenormalFlushToZero();

		disableGLReadClamp();
		stefanfw::eventHandler.subscribeToEvents(*this);

		sketch.setup();
	}

	void update()
	{
		cfg2::begin();
		stefanfw::beginFrame();
		sketch.stefanUpdate();
		sketch.stefanDraw();
		stefanfw::endFrame();
		cfg2::end();

		integratedConsole->update();
	}

	void keyDown(ci::app::KeyEvent e) {
		sketch.keyDown(e);
	}

	void mouseDrag(ci::app::MouseEvent e)
	{
		sketch.mouseDrag(e);
	}
	void mouseMove(ci::app::MouseEvent e)
	{
		sketch.mouseMove(e);
	}
};

CrossThreadCallQueue * gMainThreadCallQueue;
CINDER_APP(SketchScaffold, ci::app::RendererGl())