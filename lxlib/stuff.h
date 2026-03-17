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

#pragma once
#include "precompiled.h"
#include "util.h"

class TextureCache;


void my_assert_func(bool isTrue, string desc);
#define my_assert(isTrue) my_assert_func(isTrue, #isTrue);

void bind(gl::TextureRef& tex);
void bindTexture(gl::TextureRef& tex);
void bindTexture(gl::TextureRef tex, GLenum textureUnit);
gl::TextureRef gtex(Array2D<float> a);
gl::TextureRef gtex(Array2D<vec2> a);
gl::TextureRef gtex(Array2D<vec3> a);
gl::TextureRef gtex(Array2D<bytevec3> a);
gl::TextureRef gtex(Array2D<vec4> a);
gl::TextureRef gtex(Array2D<uvec4> a);

int sign(float f);
float expRange(float x, float min, float max);

gl::TextureRef maketex(int w, int h, GLint ifmt, bool allocateMipmaps = false, bool clear = false);

template<class T>
Array2D<T> dl(gl::TextureRef tex) {
	return Array2D<T>(); // tmp.
}

template<> Array2D<bytevec3> dl<bytevec3>(gl::TextureRef tex);
template<> Array2D<float> dl<float>(gl::TextureRef tex);
template<> Array2D<vec2> dl<vec2>(gl::TextureRef tex);
template<> Array2D<vec3> dl<vec3>(gl::TextureRef tex);
template<> Array2D<vec4> dl<vec4>(gl::TextureRef tex);

void checkGLError(string place);
#define MY_STRINGIZE_DETAIL(x) #x
#define MY_STRINGIZE(x) MY_STRINGIZE_DETAIL(x)
#define CHECK_GL_ERROR() checkGLError(__FILE__ ": " MY_STRINGIZE(__LINE__))

template<class T>
Array2D<T> gettexdata(gl::TextureRef tex, GLenum format, GLenum type) {
	Array2D<T> data(tex->getSize());
	
	bind(tex);
	glGetTexImage(GL_TEXTURE_2D, 0, format, type, data.data);
	
	return data;
}

float sq(float f);

vector<Array2D<float> > split(Array2D<vec3> arr);
void setWrapBlack(gl::TextureRef tex);

void setWrap(gl::TextureRef tex, GLenum wrap);

class FileCache {
public:
	static string get(string filename);
};

void disableGLReadClamp();

void enableDenormalFlushToZero();

template<class TVec>
TVec safeNormalized(TVec const& vec) {
	typename TVec::value_type len = length(vec);
	if (len == 0.0f) {
		return vec;
	}
	return vec / len;
}

unsigned int ilog2(unsigned int val);

vec2 compdiv(vec2 const& v1, vec2 const& v2);

void enableGlDebugOutput();
