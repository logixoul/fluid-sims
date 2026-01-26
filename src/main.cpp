#include "precompiled.h"
#include "ParticleFluidSketch.h"
#include "GridFluidSketch.h"
#include "CinderImGui.h"

struct SketchScaffold : ci::app::App {
	//ParticleFluidSketch sketch;
	GridFluidSketch sketch;

	shared_ptr<IntegratedConsole> integratedConsole;
	void setup()
	{
		::enableGlDebugOutput();

		ImGui::Initialize();
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);

		integratedConsole = make_shared<IntegratedConsole>();;

		enableDenormalFlushToZero();

		disableGLReadClamp();
		stefanfw::eventHandler.subscribeToEvents(*this);

		sketch.setup();
	}

	void update()
	{
		ImGui::Begin("Parameters");
		stefanfw::beginFrame();
		sketch.stefanUpdate();
		sketch.stefanDraw();
		stefanfw::endFrame();
		ImGui::End();

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

CINDER_APP(SketchScaffold, ci::app::RendererGl())