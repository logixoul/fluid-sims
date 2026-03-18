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

module;
#include "precompiled.h"

export module lxlib.stuff;

import lxlib.shade;

import lxlib.TextureRef;
import lxlib.util;
import lxlib.TextureCache;

// Forward declarations for functions defined in gpgpu.cpp
//void beginRTT(gl::TextureRef fbotex);
//void endRTT();

export void bind(gl::TextureRef& tex);
export void bindTexture(gl::TextureRef& tex);
export void bindTexture(gl::TextureRef tex, GLenum textureUnit);
export gl::TextureRef gtex(Array2D<float> a);
export gl::TextureRef gtex(Array2D<vec2> a);
export gl::TextureRef gtex(Array2D<vec3> a);
export gl::TextureRef gtex(Array2D<bytevec3> a);
export gl::TextureRef gtex(Array2D<vec4> a);
export gl::TextureRef gtex(Array2D<uvec4> a);

export int sign(float f);
export float expRange(float x, float min, float max);

export gl::TextureRef maketex(int w, int h, GLint ifmt, bool allocateMipmaps = false, bool clear = false);

export template<class T>
Array2D<T> dl(gl::TextureRef tex) {
	return Array2D<T>(); // tmp.
}

export template<class T>
Array2D<T> gettexdata(gl::TextureRef tex, GLenum format, GLenum type) {
	Array2D<T> data(tex->getSize());

	bind(tex);
	glGetTexImage(GL_TEXTURE_2D, 0, format, type, data.data);

	return data;
}

export template<> Array2D<bytevec3> dl<bytevec3>(gl::TextureRef tex);
export template<> Array2D<float> dl<float>(gl::TextureRef tex);
export template<> Array2D<vec2> dl<vec2>(gl::TextureRef tex);
export template<> Array2D<vec3> dl<vec3>(gl::TextureRef tex);
export template<> Array2D<vec4> dl<vec4>(gl::TextureRef tex);

export float sq(float f) {
	return f * f;
}

export void setWrapBlack(gl::TextureRef tex);

export void setWrap(gl::TextureRef tex, GLenum wrap);

export class FileCache {
public:
	static string get(string filename);
};

export void disableGLReadClamp();

export void enableDenormalFlushToZero();

export template<class TVec>
TVec safeNormalized(TVec const& vec) {
	typename TVec::value_type len = length(vec);
	if (len == 0.0f) {
		return vec;
	}
	return vec / len;
}

export unsigned int ilog2(unsigned int val);

export vec2 compdiv(vec2 const& v1, vec2 const& v2);

export void enableGlDebugOutput();

// --- Implementations ---

// tried to have this as a static member (with thread_local) but I got errors. todo.
/*thread_local*/ static std::map<string,string> FileCache_db;

static std::string readFile(const std::string& path) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		throw std::runtime_error("Failed to open file");
	}

	file.seekg(0, std::ios::end);
	std::size_t size = file.tellg();
	file.seekg(0);

	std::string buffer(size, '\0');
	file.read((char*)buffer.data(), size);

	return buffer;
}

string FileCache::get(string filename) {
	if(FileCache_db.find(filename)== FileCache_db.end()) {
		FileCache_db[filename]=readFile("../assets/"+filename);
	}
	return FileCache_db[filename];
}

static void APIENTRY messageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	if (type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP)
		return;

	cout << "GL CALLBACK. Msg: " << message << endl;

	if (type == GL_DEBUG_TYPE_ERROR) {
		cout << endl;
	}
}

void enableGlDebugOutput() {
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(messageCallback, 0);
}

void bind(gl::TextureRef & tex) {
	glBindTexture(tex->getTarget(), tex->getId());
}
void bindTexture(gl::TextureRef & tex) {
	glBindTexture(tex->getTarget(), tex->getId());
}

void bindTexture(gl::TextureRef tex, GLenum textureUnit)
{
	glActiveTexture(textureUnit);
	bindTexture(tex);
	glActiveTexture(GL_TEXTURE0); // todo: is this necessary?
}

gl::TextureRef gtex(Array2D<float> a)
{
	gl::TextureRef tex = maketex(a.w, a.h, GL_R16F);
	bind(tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RED, GL_FLOAT, a.data);
	return tex;
}

gl::TextureRef gtex(Array2D<vec2> a)
{
	gl::TextureRef tex = maketex(a.w, a.h, GL_RG16F);
	bind(tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RG, GL_FLOAT, a.data);
	return tex;
}

gl::TextureRef gtex(Array2D<vec3> a)
{
	gl::TextureRef tex = maketex(a.w, a.h, GL_RGB16F);
	bind(tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RGB, GL_FLOAT, a.data);
	return tex;
}
gl::TextureRef gtex(Array2D<bytevec3> a)
{
	gl::TextureRef tex = maketex(a.w, a.h, GL_RGB8);
	bind(tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RGB, GL_UNSIGNED_BYTE, a.data);
	return tex;
}

gl::TextureRef gtex(Array2D<vec4> a)
{
	gl::TextureRef tex = maketex(a.w, a.h, GL_RGBA16F);
	bind(tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RGBA, GL_FLOAT, a.data);
	return tex;
}

gl::TextureRef gtex(Array2D<uvec4> a)
{
	gl::TextureRef tex = maketex(a.w, a.h, GL_RGBA32UI);
	bind(tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RGBA_INTEGER, GL_UNSIGNED_INT, a.data);
	return tex;
}

int sign(float f)
{
	if (f < 0)
		return -1;
	if (f > 0)
		return 1;
	return 0;
}

float expRange(float x, float min, float max) {
	return exp(mix(std::log(min), std::log(max), x));
}

void setWrapBlack(gl::TextureRef tex) {
	// I think the border color is transparent black by default. It doesn't hurt that it is transparent.
	bind(tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, black);
}

void setWrap(gl::TextureRef tex, GLenum wrap) {
	bind(tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
}

gl::TextureRef maketex(int w, int h, GLint ifmt, bool allocateMipmaps, bool clear) {
	TextureCacheKey key;
	key.ifmt = ifmt;
	key.size = ivec2(w, h);
	key.allocateMipmaps = allocateMipmaps;
	auto tex = TextureCache::instance()->get(key);
	if(clear) {
		beginRTT(tex);
		lxClear();
		endRTT();
	}
	return tex;
}

void disableGLReadClamp() {
	glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
}

void enableDenormalFlushToZero() {
	_controlfp(_DN_FLUSH, _MCW_DN);
}

unsigned int ilog2(unsigned int val) {
	unsigned int ret = -1;
	while (val != 0) {
		val >>= 1;
		ret++;
	}
	return ret;
}

vec2 compdiv(vec2 const & v1, vec2 const & v2) {
	float a = v1.x, b = v1.y;
	float c = v2.x, d = v2.y;
	float cd = sq(c) + sq(d);
	return vec2(
		(a*c + b * d) / cd,
		(b*c - a * d) / cd);
}

template<> Array2D<bytevec3> dl<bytevec3>(gl::TextureRef tex) {
	return gettexdata<bytevec3>(tex, GL_RGB, GL_UNSIGNED_BYTE);
}

template<> Array2D<float> dl<float>(gl::TextureRef tex) {
	return gettexdata<float>(tex, GL_RED, GL_FLOAT);
}

template<> Array2D<vec2> dl<vec2>(gl::TextureRef tex) {
	return gettexdata<vec2>(tex, GL_RG, GL_FLOAT);
}

template<> Array2D<vec3> dl<vec3>(gl::TextureRef tex) {
	return gettexdata<vec3>(tex, GL_RGB, GL_FLOAT);
}

template<> Array2D<vec4> dl<vec4>(gl::TextureRef tex) {
	return gettexdata<vec4>(tex, GL_RGBA, GL_FLOAT);
}
