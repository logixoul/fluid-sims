#pragma once
/*
Tonemaster - HDR software
Copyright (C) 2018, 2019, 2020 Stefan Monov <logixoul@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

//#define BOOST_RESULT_OF_USE_DECLTYPE 
#include <cmath>
#include <iostream>
#include <string>
#include <complex>
#undef min
#undef max
#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/GlslProg.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/io.hpp>
#include "lxTextureRef.h"
#include "lxVaoVbo.h"
#include <unordered_map>
namespace gl {
	typedef lxTextureRef TextureRef;
	typedef lxTexture Texture;
	//using namespace ci::gl;
	using GlslProg = ci::gl::GlslProg;
	using GlslProgRef = ci::gl::GlslProgRef;
	using Context = ci::gl::Context;
}
#define IMGUI_USER_CONFIG "CinderImGuiConfig.h"
#define CINDER_IMGUI_EXTERNAL

using namespace glm;
using namespace std;
//namespace gl = ci::gl;
