#pragma once
#include "precompiled.h"

struct HslF
{
	float h, s, l;
	HslF(float h, float s, float l);
	HslF(glm::vec3 const& rgb);
};

vec3 FromHSL(HslF const& hsl);