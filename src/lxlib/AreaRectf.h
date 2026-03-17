#pragma once

#include "precompiled.h"

class Area {
public:
	int x1;
	int y1;
	int x2;
	int y2;
	Area(glm::ivec2 const& ul, glm::ivec2 const& br) {
		x1 = ul.x;
		y1 = ul.y;
		x2 = br.x;
		y2 = br.y;
	}
	void expand(int xe, int ye) {
		x1 -= xe;
		y1 -= ye;
		x2 += xe;
		y2 += ye;
	}
};