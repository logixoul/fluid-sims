#include "precompiled.h"
#include "ParticleFluidSketch.h"
#include "GridFluidSketch.h"
#include "MultiscaleGrayScottSketch2026.h"
#include "MyTimer.h"
#include "SketchScaffold.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

bool keys[256];
bool keys2[256];
bool mouseDown_[3];
ivec2 windowSize;

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
class SketchScaffold;

SketchScaffold* instance;

static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

struct SketchScaffold {
	//ParticleFluidSketch sketch;
	//GridFluidSketch sketch;
	MultiscaleGrayScottSketch sketch;
	GLFWwindow* window;

	shared_ptr<IntegratedConsole> integratedConsole;

	void setup()
	{
		::windowSize = ivec2(1280, 720);
		::instance = this;

		glfwSetErrorCallback(glfw_error_callback);

		if (!glfwInit())
			throw std::runtime_error("can't initialize glfw");
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

		this->window = glfwCreateWindow(::windowSize.x, ::windowSize.y, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
		if (window == nullptr)
			throw std::runtime_error("can't create window");
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1); // Enable vsync

		glfwSetCursorPosCallback(window, cursorPositionCallback);
		glfwSetMouseButtonCallback(window, mouseButtonCallback);
		
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			throw std::runtime_error("could not load glad");
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 430");
		

		integratedConsole = make_shared<IntegratedConsole>();;

		enableDenormalFlushToZero();

		disableGLReadClamp();

		sketch.setup();
	}

	// https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl3/main.cpp
	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
			{
				ImGui_ImplGlfw_Sleep(10);
				continue;
			}
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			this->update();
			
			ImGui::Render();
			glfwGetFramebufferSize(window, &::windowSize.x, &windowSize.y);
			std::clog << windowSize << std::endl;
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(window);
		}
	}

	~SketchScaffold() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void update()
	{
		ImGui::Begin("Parameters");
		
		sketch.stefanUpdate();
		sketch.stefanDraw();
		ImGui::End();

		integratedConsole->update();
	}

	/*void keyDown(ci::app::KeyEvent e) {
		keys[e.getChar()] = true;
		if (e.isControlDown() && e.getCode() != ci::app::KeyEvent::KEY_LCTRL)
		{
			keys2[e.getChar()] = !keys2[e.getChar()];
		}
		
		sketch.keyDown();
	}*/

	void mouseMove(ivec2 pos)
	{
		sketch.mouseMove(pos);
	}
	/*
	void keyUp(ci::app::KeyEvent e) {
		keys[e.getChar()] = false;
	}*/
};

static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
	instance->mouseMove(ivec2(xpos, ypos));
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	mouseDown_[button] = action == GLFW_PRESS;
}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow
) {
	SketchScaffold sketchScaffold;
	sketchScaffold.setup();
	sketchScaffold.mainLoop();
}