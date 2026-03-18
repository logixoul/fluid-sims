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
#include "macros.h"

export module lxlib.gpgpu;

import lxlib.shade;

import lxlib.TextureRef;
import lxlib.stuff;

export gl::TextureRef get_gradients_tex(gl::TextureRef src, GLuint wrap = GL_REPEAT);

export gl::TextureRef baseshade2(vector<gl::TextureRef> texv, string const& src, ShadeOpts const& opts = ShadeOpts(), string const& lib = "");
export gl::TextureRef shade2(
	gl::TextureRef tex,
	string const& src, ShadeOpts const& opts = ShadeOpts(), string const& lib = "");
export gl::TextureRef shade2(
	gl::TextureRef tex, gl::TextureRef tex2,
	string const& src, ShadeOpts const& opts = ShadeOpts(), string const& lib = "");
export gl::TextureRef shade2(
	gl::TextureRef tex, gl::TextureRef tex2, gl::TextureRef tex3,
	string const& src, ShadeOpts const& opts = ShadeOpts(), string const& lib = "");
export gl::TextureRef shade2(
	gl::TextureRef tex, gl::TextureRef tex2, gl::TextureRef tex3, gl::TextureRef tex4,
	string const& src, ShadeOpts const& opts = ShadeOpts(), string const& lib = "");
export gl::TextureRef shade2(
	gl::TextureRef tex, gl::TextureRef tex2, gl::TextureRef tex3, gl::TextureRef tex4, gl::TextureRef tex5,
	string const& src, ShadeOpts const& opts = ShadeOpts(), string const& lib = "");
export gl::TextureRef shade2(
	gl::TextureRef tex, gl::TextureRef tex2, gl::TextureRef tex3, gl::TextureRef tex4, gl::TextureRef tex5, gl::TextureRef tex6,
	string const& src, ShadeOpts const& opts = ShadeOpts(), string const& lib = "");
export gl::TextureRef gauss3tex(gl::TextureRef src);

export gl::TextureRef get_laplace_tex(gl::TextureRef src, GLuint wrap);

export struct Operable {
	explicit Operable(gl::TextureRef aTex);
	Operable operator+(gl::TextureRef other);
	Operable operator-(gl::TextureRef other);
	Operable operator*(gl::TextureRef other);
	Operable operator/(gl::TextureRef other);
	Operable operator+(float scalar) {
		return Operable(shade2(tex, "_out = fetch4() + scalar;", ShadeOpts().uniform("scalar", scalar)));
	}
	Operable operator-(float scalar) {
		return Operable(shade2(tex, "_out = fetch4() - scalar;", ShadeOpts().uniform("scalar", scalar)));
	}
	Operable operator*(float scalar) {
		return Operable(shade2(tex, "_out = fetch4() * scalar;", ShadeOpts().uniform("scalar", scalar)));
	}
	Operable operator/(float scalar) {
		return Operable(shade2(tex, "_out = fetch4() / scalar;", ShadeOpts().uniform("scalar", scalar)));
	}
	void operator+=(gl::TextureRef other);
	void operator-=(gl::TextureRef other);
	void operator*=(gl::TextureRef other);
	void operator/=(gl::TextureRef other);
	float dot(gl::TextureRef other);
	gl::TextureRef dotTex(gl::TextureRef other);
	operator gl::TextureRef();
private:
	gl::TextureRef tex;
};

export Operable op(gl::TextureRef tex);

// --- Implementations from gpgpu.cpp ---

gl::TextureRef get_gradients_tex(gl::TextureRef src, GLuint wrap) {
	GPU_SCOPE("get_gradients_tex");
	glActiveTexture(GL_TEXTURE0);
	::bindTexture(src);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	return shade2(src,
		"	float srcL=fetch1(tex,tc+tsize*vec2(-1.0,0.0));"
		"	float srcR=fetch1(tex,tc+tsize*vec2(1.0,0.0));"
		"	float srcT=fetch1(tex,tc+tsize*vec2(0.0,-1.0));"
		"	float srcB=fetch1(tex,tc+tsize*vec2(0.0,1.0));"
		"	float dx=(srcR-srcL)/2.0;"
		"	float dy=(srcB-srcT)/2.0;"
		"	_out.xy=vec2(dx,dy);"
		,
		ShadeOpts().ifmt(GL_RG16F)
	);
}

inline gl::TextureRef baseshade2(vector<gl::TextureRef> texv, string const& src, ShadeOpts const& opts, string const& lib)
{
	return shade(texv, lib + "void shade() {" + src + "}", opts);
}

gl::TextureRef shade2(gl::TextureRef tex, string const& src, ShadeOpts const& opts, string const& lib)
{
	return baseshade2({ tex }, src, opts, lib);
}

gl::TextureRef shade2(gl::TextureRef tex, gl::TextureRef tex2, string const& src, ShadeOpts const& opts, string const& lib)
{
	return baseshade2({ tex,tex2 }, src, opts, lib);
}

gl::TextureRef shade2(gl::TextureRef tex, gl::TextureRef tex2, gl::TextureRef tex3, string const& src, ShadeOpts const& opts, string const& lib)
{
	return baseshade2({ tex, tex2, tex3 }, src, opts, lib);
}

gl::TextureRef shade2(gl::TextureRef tex, gl::TextureRef tex2, gl::TextureRef tex3, gl::TextureRef tex4, string const& src, ShadeOpts const& opts, string const& lib)
{
	return baseshade2({ tex, tex2, tex3, tex4 }, src, opts, lib);
}

gl::TextureRef shade2(gl::TextureRef tex, gl::TextureRef tex2, gl::TextureRef tex3, gl::TextureRef tex4, gl::TextureRef tex5, string const& src, ShadeOpts const& opts, string const& lib)
{
	return baseshade2({ tex, tex2, tex3, tex4, tex5 }, src, opts, lib);
}

gl::TextureRef shade2(gl::TextureRef tex, gl::TextureRef tex2, gl::TextureRef tex3, gl::TextureRef tex4, gl::TextureRef tex5, gl::TextureRef tex6, string const& src, ShadeOpts const& opts, string const& lib)
{
	return baseshade2({ tex, tex2, tex3, tex4, tex5, tex6 }, src, opts, lib);
}

gl::TextureRef gauss3tex(gl::TextureRef src) {
	auto state = shade2(src,
		"vec3 sum = vec3(0.0);"
		"sum += fetch3(tex, tc + tsize * vec2(-1.0, -1.0)) / 16.0;"
		"sum += fetch3(tex, tc + tsize * vec2(-1.0, 0.0)) / 8.0;"
		"sum += fetch3(tex, tc + tsize * vec2(-1.0, +1.0)) / 16.0;"

		"sum += fetch3(tex, tc + tsize * vec2(0.0, -1.0)) / 8.0;"
		"sum += fetch3(tex, tc + tsize * vec2(0.0, 0.0)) / 4.0;"
		"sum += fetch3(tex, tc + tsize * vec2(0.0, +1.0)) / 8.0;"

		"sum += fetch3(tex, tc + tsize * vec2(+1.0, -1.0)) / 16.0;"
		"sum += fetch3(tex, tc + tsize * vec2(+1.0, 0.0)) / 8.0;"
		"sum += fetch3(tex, tc + tsize * vec2(+1.0, +1.0)) / 16.0;"
		"_out.rgb = sum;"
		);
	return state;
}

gl::TextureRef get_laplace_tex(gl::TextureRef src, GLuint wrap) {
	glActiveTexture(GL_TEXTURE0);
	::bindTexture(src);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	auto state = shade2(src,
		"vec3 sum = vec3(0.0);"
		"sum += fetch3(tex, tc + tsize * vec2(-1.0, 0.0)) * -1.0;"
		"sum += fetch3(tex, tc + tsize * vec2(0.0, -1.0)) * -1.0;"
		"sum += fetch3(tex, tc + tsize * vec2(0.0, +1.0)) * -1.0;"
		"sum += fetch3(tex, tc + tsize * vec2(+1.0, 0.0)) * -1.0;"
		"sum += fetch3(tex, tc + tsize * vec2(0.0, 0.0)) * 4.0;"
		"_out.rgb = -sum;"
		);
	return state;
}

Operable op(gl::TextureRef tex) {
	return Operable(tex);
}

inline Operable::Operable(gl::TextureRef aTex) {
	tex = aTex;
}

Operable Operable::operator+(gl::TextureRef other) {
	return Operable(shade2(tex, other, "_out = fetch4() + fetch4(tex2);"));
}
Operable Operable::operator-(gl::TextureRef other) {
	return Operable(shade2(tex, other, "_out = fetch4() - fetch4(tex2);"));
}
Operable Operable::operator*(gl::TextureRef other) {
	return Operable(shade2(tex, other, "_out = fetch4() * fetch4(tex2);"));
}
Operable Operable::operator/(gl::TextureRef other) {
	return Operable(shade2(tex, other, "_out = fetch4() / fetch4(tex2);"));
}

void Operable::operator+=(gl::TextureRef other) {
	tex = shade2(tex, other, "_out = fetch4() + fetch4(tex2);");
}
void Operable::operator-=(gl::TextureRef other) {
	tex = shade2(tex, other, "_out = fetch4() - fetch4(tex2);");
}
void Operable::operator*=(gl::TextureRef other) {
	tex = shade2(tex, other, "_out = fetch4() * fetch4(tex2);");
}
void Operable::operator/=(gl::TextureRef other) {
	tex = shade2(tex, other, "_out = fetch4() / fetch4(tex2);");
}

Operable::operator gl::TextureRef() {
	return tex;
}
