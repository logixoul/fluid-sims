#pragma once

#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <mutex>
#include <functional>
#include <fstream>
#include <map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// glm doesn't compile unless I I define the following...
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "lxGlslProg.h"
#include <glm/gtx/io.hpp>
#include "lxTextureRef.h"
#include "lxVaoVbo.h"
#include "lxAreaRectf.h"
#include <unordered_map>
#include "imgui.h"


namespace gl {
	typedef lxTextureRef TextureRef;
	typedef lxTexture Texture;
}

using namespace glm;
using namespace std;
