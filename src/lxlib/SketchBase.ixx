module;
#include "precompiled.h"
#include <glm/vec2.hpp>

export module lxlib.SketchBase;

export struct SketchBase {
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
