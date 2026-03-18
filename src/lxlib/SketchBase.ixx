module;
#include "precompiled.h"
#include <glm/vec2.hpp>

export module lxlib.SketchBase;

export struct SketchBase {
	std::vector<bool> keys;
	std::vector<bool> mouseDown_;
	glm::ivec2 windowSize;
	SketchBase() {
		keys.resize(256);
		mouseDown_.resize(3);
	}
	
	virtual void keyDown(int key) {
	}
	virtual void keyUp(int key) {
	}
	virtual void mouseMove(glm::ivec2 pos) {
	}
	virtual void draw() {
	}
	virtual void update() {
	}
	virtual void setup() {
	}
};
