#include "precompiled.h"
#include "ParticleFluidSketch.h"
#include "GridFluidSketch.h"
#include "CinderImGui.h"
#include "MyTimer.h"
#include "SketchScaffold.h"

bool keys[256];
bool keys2[256];
bool mouseDown_[3];

struct SketchScaffold : ci::app::App {
	//ParticleFluidSketch sketch;
	GridFluidSketch sketch;

	shared_ptr<IntegratedConsole> integratedConsole;
	void setup()
	{
		ci::app::setWindowSize(1280, 720);

		::enableGlDebugOutput();

		ImGui::Initialize();
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);

		integratedConsole = make_shared<IntegratedConsole>();;

		enableDenormalFlushToZero();

		disableGLReadClamp();

		sketch.setup();
	}

	void update()
	{
		ImGui::Begin("Parameters");
		
		sketch.stefanUpdate();
		sketch.stefanDraw();
		TimerManager::update();
		ImGui::End();

		integratedConsole->update();
	}

	void keyDown(ci::app::KeyEvent e) {
		keys[e.getChar()] = true;
		if (e.isControlDown() && e.getCode() != ci::app::KeyEvent::KEY_LCTRL)
		{
			keys2[e.getChar()] = !keys2[e.getChar()];
		}
		
		sketch.keyDown();
	}

	void mouseDrag(ci::app::MouseEvent e)
	{
		sketch.mouseDrag(e.getPos());
	}
	void mouseMove(ci::app::MouseEvent e)
	{
		sketch.mouseMove(e.getPos());
	}

	void keyUp(ci::app::KeyEvent e) {
		keys[e.getChar()] = false;
	}

	void mouseDown(ci::app::MouseEvent e)
	{
		mouseDown_[e.isLeft() ? 0 : e.isMiddle() ? 1 : 2] = true;
	}
	void mouseUp(ci::app::MouseEvent e)
	{
		mouseDown_[e.isLeft() ? 0 : e.isMiddle() ? 1 : 2] = false;
	}
};

CINDER_APP(SketchScaffold, ci::app::RendererGl())