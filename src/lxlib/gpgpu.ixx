module;
#include "precompiled.h"
#include "macros.h"

export module lxlib.gpgpu;

import lxlib.shade;

import lxlib.TextureRef;
import lxlib.stuff;

export gl::TextureRef getGradients(gl::TextureRef src, GLuint wrap = GL_REPEAT);

export gl::TextureRef gaussianBlur3x3(gl::TextureRef src);

export gl::TextureRef getLaplace(gl::TextureRef src, GLuint wrap);

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

gl::TextureRef getGradients(gl::TextureRef src, GLuint wrap) {
	GPU_SCOPE("getGradients");
	glActiveTexture(GL_TEXTURE0);
	src->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	return shade(src,
         "	float srcL=texture(tex,texCoord+tsize*vec2(-1.0,0.0)).x;"
			"	float srcR=texture(tex,texCoord+tsize*vec2(1.0,0.0)).x;"
			"	float srcT=texture(tex,texCoord+tsize*vec2(0.0,-1.0)).x;"
			"	float srcB=texture(tex,texCoord+tsize*vec2(0.0,1.0)).x;"
		"	float dx=(srcR-srcL)/2.0;"
		"	float dy=(srcB-srcT)/2.0;"
		"	_out.xy=vec2(dx,dy);"
		,
		ShadeOpts().ifmt(GL_RG16F)
	);
}

gl::TextureRef gaussianBlur3x3(gl::TextureRef src) {
	auto state = shade(src,
		"vec4 sum = vec4(0.0);"
            "sum += texture(tex, texCoord + tsize * vec2(-1.0, -1.0)) / 16.0;"
			"sum += texture(tex, texCoord + tsize * vec2(-1.0, 0.0)) / 8.0;"
			"sum += texture(tex, texCoord + tsize * vec2(-1.0, +1.0)) / 16.0;"

          "sum += texture(tex, texCoord + tsize * vec2(0.0, -1.0)) / 8.0;"
			"sum += texture(tex, texCoord + tsize * vec2(0.0, 0.0)) / 4.0;"
			"sum += texture(tex, texCoord + tsize * vec2(0.0, +1.0)) / 8.0;"

            "sum += texture(tex, texCoord + tsize * vec2(+1.0, -1.0)) / 16.0;"
			"sum += texture(tex, texCoord + tsize * vec2(+1.0, 0.0)) / 8.0;"
			"sum += texture(tex, texCoord + tsize * vec2(+1.0, +1.0)) / 16.0;"
		"_out = sum;"
		);
	return state;
}

gl::TextureRef getLaplace(gl::TextureRef src, GLuint wrap) {
	glActiveTexture(GL_TEXTURE0);
	src->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	auto state = shade(src,
		"vec4 sum = vec4(0.0);"
     "sum += texture(tex, texCoord + tsize * vec2(-1.0, 0.0)) * -1.0;"
		"sum += texture(tex, texCoord + tsize * vec2(0.0, -1.0)) * -1.0;"
		"sum += texture(tex, texCoord + tsize * vec2(0.0, +1.0)) * -1.0;"
		"sum += texture(tex, texCoord + tsize * vec2(+1.0, 0.0)) * -1.0;"
		"sum += texture(tex, texCoord + tsize * vec2(0.0, 0.0)) * 4.0;"
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
