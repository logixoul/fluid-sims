module;
#include "precompiled.h"
#include "macros.h"

export module lxlib.gpgpu;

import lxlib.shade;

import lxlib.TextureRef;
import lxlib.stuff;

export gl::TextureRef get_gradients_tex(gl::TextureRef src, GLuint wrap = GL_REPEAT);

export gl::TextureRef gauss3tex(gl::TextureRef src);

export gl::TextureRef get_laplace_tex(gl::TextureRef src, GLuint wrap);

export struct Operable {
	explicit Operable(gl::TextureRef aTex);
	Operable operator+(gl::TextureRef other);
	Operable operator-(gl::TextureRef other);
	Operable operator*(gl::TextureRef other);
	Operable operator/(gl::TextureRef other);
	Operable operator+(float scalar) {
		return Operable(shade(tex, "_out = texture() + scalar;", ShadeOpts().uniform("scalar", scalar)));
	}
	Operable operator-(float scalar) {
		return Operable(shade(tex, "_out = texture() - scalar;", ShadeOpts().uniform("scalar", scalar)));
	}
	Operable operator*(float scalar) {
		return Operable(shade(tex, "_out = texture() * scalar;", ShadeOpts().uniform("scalar", scalar)));
	}
	Operable operator/(float scalar) {
		return Operable(shade(tex, "_out = texture() / scalar;", ShadeOpts().uniform("scalar", scalar)));
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
	return shade(src,
		"	float srcL=texture(tex,tc+tsize*vec2(-1.0,0.0)).x;"
		"	float srcR=texture(tex,tc+tsize*vec2(1.0,0.0)).x;"
		"	float srcT=texture(tex,tc+tsize*vec2(0.0,-1.0)).x;"
		"	float srcB=texture(tex,tc+tsize*vec2(0.0,1.0)).x;"
		"	float dx=(srcR-srcL)/2.0;"
		"	float dy=(srcB-srcT)/2.0;"
		"	_out.xy=vec2(dx,dy);"
		,
		ShadeOpts().ifmt(GL_RG16F)
	);
}

gl::TextureRef gauss3tex(gl::TextureRef src) {
	auto state = shade(src,
		"vec4 sum = vec4(0.0);"
		"sum += texture(tex, tc + tsize * vec2(-1.0, -1.0)) / 16.0;"
		"sum += texture(tex, tc + tsize * vec2(-1.0, 0.0)) / 8.0;"
		"sum += texture(tex, tc + tsize * vec2(-1.0, +1.0)) / 16.0;"

		"sum += texture(tex, tc + tsize * vec2(0.0, -1.0)) / 8.0;"
		"sum += texture(tex, tc + tsize * vec2(0.0, 0.0)) / 4.0;"
		"sum += texture(tex, tc + tsize * vec2(0.0, +1.0)) / 8.0;"

		"sum += texture(tex, tc + tsize * vec2(+1.0, -1.0)) / 16.0;"
		"sum += texture(tex, tc + tsize * vec2(+1.0, 0.0)) / 8.0;"
		"sum += texture(tex, tc + tsize * vec2(+1.0, +1.0)) / 16.0;"
		"_out = sum;"
		);
	return state;
}

gl::TextureRef get_laplace_tex(gl::TextureRef src, GLuint wrap) {
	glActiveTexture(GL_TEXTURE0);
	::bindTexture(src);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	auto state = shade(src,
		"vec4 sum = vec4(0.0);"
		"sum += texture(tex, tc + tsize * vec2(-1.0, 0.0)) * -1.0;"
		"sum += texture(tex, tc + tsize * vec2(0.0, -1.0)) * -1.0;"
		"sum += texture(tex, tc + tsize * vec2(0.0, +1.0)) * -1.0;"
		"sum += texture(tex, tc + tsize * vec2(+1.0, 0.0)) * -1.0;"
		"sum += texture(tex, tc + tsize * vec2(0.0, 0.0)) * 4.0;"
		"_out = -sum;"
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
	return Operable(shade({ tex, other }, "_out = texture() + texture(tex2);"));
}
Operable Operable::operator-(gl::TextureRef other) {
	return Operable(shade({ tex, other }, "_out = texture() - texture(tex2);"));
}
Operable Operable::operator*(gl::TextureRef other) {
	return Operable(shade({ tex, other }, "_out = texture() * texture(tex2);"));
}
Operable Operable::operator/(gl::TextureRef other) {
	return Operable(shade({ tex, other }, "_out = texture() / texture(tex2);"));
}

void Operable::operator+=(gl::TextureRef other) {
	tex = shade({ tex, other }, "_out = texture() + texture(tex2);");
}
void Operable::operator-=(gl::TextureRef other) {
	tex = shade({ tex, other }, "_out = texture() - texture(tex2);");
}
void Operable::operator*=(gl::TextureRef other) {
	tex = shade({ tex, other }, "_out = texture() * texture(tex2);");
}
void Operable::operator/=(gl::TextureRef other) {
	tex = shade({ tex, other }, "_out = texture() / texture(tex2);");
}

Operable::operator gl::TextureRef() {
	return tex;
}
