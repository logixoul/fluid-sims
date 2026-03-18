#include "precompiled.h"
#if 0
import lxlib.util;
import lxlib.stuff;
import lxlib.TextureRef;
import lxlib.gpgpu;
import lxlib.gpuBlur2_5;
import lxlib.SketchBase;
#include <lxlib/shade.h>
#include <lxlib/simplexnoise.h>

#include <lxlib/colorspaces.h>

int wsx = 1280, wsy = 720;
int scale = 1;
int sx = wsx / ::scale;
int sy = wsy / ::scale;

float noiseTimeDim = 0.0f;
const int MAX_AGE = 100;

bool pause;
const double M_PI = 3.14159265359;
vec3 complexToColor_HSV(vec2 comp) {
	float hue = (float)M_PI + (float)atan2(comp.y, comp.x);
	hue /= (float)(2 * M_PI);
	float lightness = length(comp);
	lightness = .5f;
	//lightness /= lightness + 1.0f;
	HslF hsl(hue, 1.0f, lightness);
	return FromHSL(hsl);
}

struct Walker {
	vec2 pos;
	int age;
	vec3 color;
	vec2 lastMove;

	float alpha() {
		return std::min((age / (float)MAX_AGE) * 5.0, 1.0);
	}

	Walker() {
		pos = vec2(randFloat()* sx, randFloat()*sy);
		age = std::rand() % MAX_AGE;
	}
	float noiseXAt(vec2 p) {
		int numDetailsX = 5;
		float nscale = numDetailsX / (float)sx;
		float noiseX = ::octave_noise_3d(3, .5, 1.0, p.x * nscale, p.y * nscale, noiseTimeDim);
		return noiseX;
	}

	float noiseYAt(vec2 p) {
		int numDetailsX = 5;
		float nscale = numDetailsX / (float)sx;
		float noiseY = ::octave_noise_3d(3, .5, 1.0, p.x * nscale, p.y * nscale + numDetailsX, noiseTimeDim);
		return noiseY;
	}
	vec2 noisevec2At(vec2 p) {
		return vec2(noiseXAt(p), noiseYAt(p));
	}
	vec2 curlNoisevec2At(vec2 p) {
		float eps = 1;
		float noiseXAbove = noiseXAt(p - vec2(0, eps));
		float noiseXBelow = noiseXAt(p + vec2(0, eps));
		float noiseYOnLeft = noiseYAt(p - vec2(eps, 0));
		float noiseYOnRight = noiseYAt(p + vec2(eps, 0));
		return vec2(noiseXBelow - noiseXAbove, -(noiseYOnRight - noiseYOnLeft)) / (2.0f * eps);
	}
	void update() {
		vec2 toAdd = curlNoisevec2At(pos) * 50.0f;
		//toAdd.y -= 1.0f;
		pos += toAdd / float(::scale);
		color = complexToColor_HSV(toAdd);
		//color *= min(1.0f, age / (MAX_AGE / 40.0f));
		lastMove = toAdd;
		//color = vec3::one();

		if (pos.x < 0) pos.x += sx;
		if (pos.y < 0) pos.y += sy;
		pos.x = fmod(pos.x, sx);
		pos.y = fmod(pos.y, sy);

		age++;
	}
};

vector<Walker> walkers;

void updateConfig() {
}

struct ParticleTraces2DSketch : public SketchBase {
	void setup()
	{
		enableDenormalFlushToZero();

		disableGLReadClamp();

		for (int i = 0; i < 4000 / sq(::scale); i++) {
			walkers.push_back(Walker());
		}
	}
	void keyDown(int key)
	{

		if (keys['p'] || keys['2'])
		{
			pause = !pause;
		}
	}
	float noiseProgressSpeed;

	void stefanUpdate() {
		noiseProgressSpeed = .00008f;

		if (!pause) {
			noiseTimeDim += noiseProgressSpeed;

			foreach(Walker & walker, walkers) {
				walker.update();
				if (walker.age > MAX_AGE) {
					walker = Walker();
				}
			}
		}

		if (pause)
			Sleep(50);
	}

	void stefanDraw() {
		auto t = getElapsedFrames() * 0.01f;
		//mat3 rotMat3 = glm::rotate(rotMat3, std::sin(t / 10.0f));
		//mat2 rotMat = mat2(rotMat3);
		auto refVec = vec2(sinf(t), cosf(t));

		gl::clear(Color(0, 0, 0));
		static Array2D<vec3> sizeSource(sx, sy);
		static auto sizeSourceTex = gtex(sizeSource);
		string bg = "vec3 bg = vec3(0.0);";
		static auto walkerTex = shade2(sizeSourceTex, "_out.rgb = bg;", ShadeOpts(), bg);
		if (!pause) {
			walkerTex = shade2(walkerTex, "_out.rgb = mix(fetch3(), bg, 0.007);", ShadeOpts(), bg);

			glPointSize(2.5);
			std::vector<vec4> color;
			std::vector<vec2> pos;
			{
				foreach(Walker & walker, walkers) {
					auto walkerColor = walker.color;
					float hueDot = dot(refVec, safeNormalized(walker.lastMove));
					hueDot = max(0.0f, hueDot);
					hueDot = max(0.0f, 1 - hueDot);
					walkerColor *= hueDot;
					auto c = vec4(walkerColor, walker.alpha());

					color.push_back(c); pos.push_back(walker.pos);
				}
				//gl::popMatrices();
			}
			{
				gl::ScopedBlend sb1(true);
				gl::ScopedBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				gl::ScopedViewport(0, 0, sx, sy);
				//gl::ScopedBlend(GL_SRC_ALPHA, GL_ONE);
				gl::pushMatrices();
				gl::setMatricesWindow(sx, sy, true);
				static auto colorDef = gl::ShaderDef().color();
				static auto colorProg = gl::getStockShader(colorDef);
				gl::ScopedGlslProg sgp(colorProg);

				beginRTT(walkerTex);
				{
					gl::VboRef colorVbo = gl::Vbo::create(GL_ARRAY_BUFFER, color, GL_STATIC_DRAW);
					gl::VboRef posVbo = gl::Vbo::create(GL_ARRAY_BUFFER, pos, GL_STATIC_DRAW);
					geom::BufferLayout colorLayout, posLayout;
					colorLayout.append(geom::COLOR, 4, sizeof(decltype(color[0])), 0);
					posLayout.append(geom::POSITION, 2, sizeof(decltype(pos[0])), 0);

					gl::VboMeshRef vboMesh = gl::VboMesh::create(color.size(), GL_POINTS,
						{ std::make_pair(colorLayout, colorVbo), std::make_pair(posLayout, posVbo) });
					//glUseProgram(0);
					gl::draw(vboMesh);
				}
				endRTT();
			}
		}
		auto walkerTexThres = shade2(walkerTex,
			"vec3 c = fetch3();"
			"float avg = dot(c, vec3(1)/3.0f);"
			"if(avg < .25)"
			"	 c = vec3(0);"
			"_out.rgb = c;"
		);
		auto walkerTexB = gpuBlur2_5::run(walkerTexThres, 4);
		auto walkerTex2 = shade2(walkerTex, walkerTexB,
			"vec3 c = fetch3();"
			"vec3 hsl = rgb2hsl(c);"
			"hsl.z /= .5;"
			"hsl.z = min(hsl.z, 1.0);"
			"hsl.z = pow(hsl.z, 3.0);"
			"c = hsl2rgb(hsl);"
			"c += fetch3(tex2);"
			"_out.rgb = c;",
			ShadeOpts().ifmt(GL_RGB32F),
			FileCache::get("stuff.fs")
		);
		lxDraw(walkerTex2);
	}
};
#endif