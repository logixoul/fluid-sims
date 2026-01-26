#include "precompiled.h"
#include "stuff.h"
#include "shade.h"
#include "gpgpu.h"
#include "gpuBlur2_5.h"
#include "stefanfw.h"
#include "Array2D_imageProc.h"
#include "cfg1.h"
#include "cfg2.h"
#include "IntegratedConsole.h"

#include "util.h"

struct ParticleFluidSketch {
	typedef Array2D<float> Image;
	const int scale = 2;
	ivec2 sz;

	struct Particle {
		vec2 pos;
		vec2 velocity;
		vec2 force;
		float densityHere;
		float pressureHere;
		vec2 velocityContribs;
		float velocityContribsSumWeights;
	};

	vector<Particle> particles;

	void reset() {
		particles.clear();
	}

	bool pause = false;

	void setup()
	{
		ci::app::setWindowSize(800, 800);

		sz = ivec2(ci::app::getWindowWidth() / scale, ci::app::getWindowHeight() / scale);

		reset();
	}
	void keyDown(ci::app::KeyEvent e)
	{
		if (e.getChar() == 'd')
		{
			//cfg2::params->isVisible() ? cfg2::params->hide() : cfg2::params->show();
		}

		if (keys[' ']) {
			doFluidStep();
		}
		if (keys['r'])
		{
			reset();
		}
		if (keys['p'] || keys['2'])
		{
			pause = !pause;
		}
	}
	vec2 direction;
	vec2 lastm;
	void mouseDrag(ci::app::MouseEvent e)
	{
		mm(e);
	}
	void mouseMove(ci::app::MouseEvent e)
	{
		mm(e);
	}
	void mm(ci::app::MouseEvent e)
	{
		direction = vec2(e.getPos()) - lastm;
		lastm = e.getPos();
	}
	float surfaceTensionThreshold = 1.0f;
	float pushawayCoef = .2f;
	float surfaceTensionCoef = 0.2f;
	void stefanDraw()
	{
		static float blurSize = 1.41f;
		ImGui::DragFloat("blurSize", &blurSize, 1.0f, 0.1, 100, "%.3f", ImGuiSliderFlags_Logarithmic);
		static int blurIters = 3;
		ImGui::DragInt("blurIters", &blurIters, 1.0f, 1, 16, "%d", ImGuiSliderFlags_None);
		static float gradScale = 1.68;
		ImGui::DragFloat("gradScale", &gradScale, 1.0f, 1, 2000, "%.3f", ImGuiSliderFlags_Logarithmic);
		static float renderThreshold = 0.07f;
		ImGui::DragFloat("renderThreshold", &renderThreshold, 1.0f, .001f, 20, "%.3f", ImGuiSliderFlags_Logarithmic);

		ImGui::DragFloat("surfaceTensionThreshold", &surfaceTensionThreshold, .1f, .001f, 20.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
		ImGui::DragFloat("pushawayCoef", &pushawayCoef, .1f, .001f, 20.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
		ImGui::DragFloat("surfaceTensionCoef", &surfaceTensionCoef, .1f, .001f, 20.0f, "%.3f", ImGuiSliderFlags_Logarithmic);

		static float bloomSize = 1.5f;
		static int bloomIters = 6.0f;
		static float bloomIntensity = 0.07f;
		ImGui::DragFloat("bloomSize", &bloomSize, 1.0f, 0.1, 100, "%.3f", ImGuiSliderFlags_Logarithmic);
		ImGui::DragInt("bloomIters", &bloomIters, 1.0f, 1, 16, "%d", ImGuiSliderFlags_None);
		ImGui::DragFloat("bloomIntensity", &bloomIntensity, 1.0f, 0.0001, 2000, "%.3f", ImGuiSliderFlags_Logarithmic);

		gl::clear(Color(0, 0, 0));

		gl::setMatricesWindow(ci::app::getWindowSize(), false);
		auto img = Array2D<float>(sz);
		for (auto& particle : particles) {
			aaPoint<float, WrapModes::GetClamped>(img, particle.pos, 47);
		}

		auto tex = gtex(img);
		auto texB = gpuBlur2_5::run(tex, blurIters);
		auto thresholdedTex = shade2(texB,
			"float f = fetch1();"
			"float fw = fwidth(f);"
			"f = smoothstep(0.15-fw/2, 0.15+fw/2, f);"
			"_out.r = f;");

		auto limitedTex = shade2(texB,
			"float f = fetch1();"
			"float fw = fwidth(f);"
			"f *= smoothstep(renderThreshold-fw/2, renderThreshold+fw/2, f);"
			"_out.r = f;",
			ShadeOpts().uniform("renderThreshold", renderThreshold)
		);

		auto redTex = thresholdedTex;

		auto redTexB = gpuBlur2_5::run_longtail(redTex, bloomIters, bloomSize);

		redTex = shade2(redTex, redTexB, MULTILINE(
			_out.r = (fetch1() + fetch1(tex2)) * bloomIntensity;
		),
			ShadeOpts().uniform("bloomIntensity", bloomIntensity)
		);
		/*greenTex = shade2(greenTex, greenTexB, MULTILINE(
			_out.r = (fetch1() + fetch1(tex2)) * bloomIntensity;
		),
			ShadeOpts().uniform("bloomIntensity", bloomIntensity)
			);*/
		static auto envMap = gl::Texture::create(ci::loadImage(ci::app::loadAsset("milkyway.png")));
		//static auto envMap = gl::TextureCubeMap::create(loadImage(loadAsset("envmap_cube.jpg")), gl::TextureCubeMap::Format().mipmap());
		//gl::TextureBaseRef test=envMap;
		auto grads = get_gradients_tex(limitedTex);

		auto tex2 = shade2(texB, grads, envMap, redTex, /*greenTex,*/
			"vec2 grad = fetch2(tex2) * gradScale;"
			"vec3 N = normalize(vec3(-grad.x, -grad.y, -1.0));"
			"vec3 I=-normalize(vec3(tc.x-.5, tc.y-.5, 0.2));"
			"float eta=1.0/1.3;"
			"vec3 R=refract(I, N, eta);"
			"vec3 c = getEnv(R);"
			"c*=0.5;//*pow(.9, fetch1(tex) * 50.0);\n"
			//"vec3 c = N;"
			"vec3 albedo = 0.0*vec3(0.005, 0.0, 0.0);"
			//"c = mix(albedo, c, pow(.9, fetch1(tex) * 50.0));" // tmp
			"R = reflect(I, N);"
			"if(fetch1(tex) > surfTensionThres) {"
			//"	vec3 refl = reflect(-viewDir, normal);"
			"	vec3 refl = R;"
			"	vec3 normal = N;"
			"	vec3 viewDir = -I;"
			"	float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 5.0);"
			"	float fresnelWeight = mix(0.1, 1.0, fresnel);"
			"	c += getEnv(refl) * fresnelWeight;"
			"}"

			"float redVal = fetch1(tex4);"
			"redVal = 1.0-exp(-redVal);"
			//"redVal = min(3.0, redVal);"
			//"float greenVal = fetch1(tex5);"
			// this is taken from https://www.shadertoy.com/view/Mld3Rn
			"vec3 redColor = vec3(min(redVal * 1.5, 1.), pow(redVal, 2.5), pow(redVal, 12.)); "
			//"vec3 greenColor = vec3(min(greenVal * 1.5, 1.), pow(greenVal, 2.5), pow(greenVal, 12.)).zyx; "
			"c += redColor;"
			//"c += greenColor;"

			"_out.rgb = c;"
			, ShadeOpts().ifmt(GL_RGB16F).scale(4.0f).uniform("surfTensionThres", 0.1f).uniform("gradScale", gradScale),
			"float PI = 3.14159265358979323846264;\n"
			"vec2 latlong(vec3 refl) {\n"
			"	return vec2(atan(refl.z, refl.x) / (2.0 * PI) + 0.5, asin(clamp(refl.y, -1.0, 1.0)) / PI + 0.5);"
			"}\n"
			"vec3 getEnv(vec3 v) {\n"
			"	vec3 c = fetch3(tex3, latlong(v));\n"
			"	c = pow(c, vec3(2.2));" // gamma correction
			"	return c;"
			"}\n"
		);

		tex2 = shade2(tex2,
			"vec3 c = fetch3(tex);"
			"if(c.r<0.0||c.g<0.0||c.b<0.0) { _out.rgb = vec3(1.0, 0.0, 0.0); }" // eases debugging
			"c = pow(c, vec3(1.0/2.2));"
			"_out.rgb = c;"
		);

		/*tex2 = tex;
		tex2 = shade2(tex2,
			"float f = fetch1();"
			"float fw = fwidth(f);"
			"f = smoothstep(0.5-fw/2, 0.5+fw/2, f);"
			//"f = dFdx(f)+dFdy(f);"
			"_out.rgb = vec3(f);"
			, ShadeOpts().ifmt(GL_RGB16F));*/

			//videoWriter->write(tex2);

		gl::draw(tex2, ci::app::getWindowBounds());

		/*gl::setMatricesWindow(getWindowSize(), true);
		for (auto& p : particles) {
			gl::color(Color::white());
			gl::drawSolidCircle(p.pos * float(::scale), 4);
		}*/
	}
	void stefanUpdate()
	{
		if (!pause)
		{
			doFluidStep();

		} // if ! pause
		vec2 mousePos = this->lastm;
		mousePos /= scale;
		if (mouseDown_[0])
		{
			float t = ci::app::getElapsedFrames();

			Particle part; part.pos = mousePos + vec2(sin(t), cos(t)) * 30.0f;
			particles.push_back(part);
		}
		else if (mouseDown_[2]) {
			for (Particle& part : particles) {
				if (distance(part.pos, mousePos) < 40) {
					const float velocityScaleFactor = 0.6f / (float)scale;
					part.velocity += velocityScaleFactor * direction;
					float speed = glm::length(part.velocity);
					float newSpeed = std::min(speed, velocityScaleFactor * 30);
					part.velocity = part.velocity * newSpeed / speed;
				}
			}
		}
	}

	float steepKernel(float dist, float radius) {
		if (dist >= radius) return 0.0f;
		float x = 1.0f - dist / radius;
		return x * x;
	}

	float smoothKernel(float dist, float radius) {
		if (dist >= radius) return 0.0f;
		return glm::smoothstep(1.0f, 0.0f, dist / radius);
	}

	const float MAX_DIST = 20;
	void forEachNeighbourPair(std::function<void(Particle&, Particle&, vec2 const&, float)> const& cb) {
		for (int i = 0; i < particles.size(); i++) {
			auto& p1 = particles[i];
			for (int j = i + 1; j < particles.size(); j++) {
				auto& p2 = particles[j];
				auto vec = p1.pos - p2.pos;
				float dist = length(vec);
				if (dist <= MAX_DIST) {
					cb(p1, p2, vec, dist);
				}
			}
		}
	}

	void lxComputeForces() {
		for (auto& p : particles) {
			p.densityHere = 0;
			p.pressureHere = 0;
		}

		forEachNeighbourPair([&](auto& p1, auto& p2, vec2 const& vec, float dist) {
			auto vecNorm = vec / dist; // normalized
			float f = steepKernel(dist, MAX_DIST);

			float densityToAdd = f;
			p1.densityHere += densityToAdd;
			p2.densityHere += densityToAdd;
			});

		for (auto& p : particles) {
			p.pressureHere = p.densityHere - surfaceTensionThreshold;
		}

		forEachNeighbourPair([&](auto& p1, auto& p2, vec2 const& vec, float dist) {
			auto vecNorm = vec / (dist + 0.0001f); // normalized
			float f = steepKernel(dist, MAX_DIST);
			vec2 pushawayVec = vecNorm * f;
			const float pressureSum = p1.pressureHere + p2.pressureHere;
			p1.force += pushawayVec * surfaceTensionCoef * pressureSum;
			p2.force -= pushawayVec * surfaceTensionCoef * pressureSum;
			});

		smoothenVelocities();
	}

	void smoothenVelocities() {
		for (auto& p : particles) {
			float f = smoothKernel(0.0f, MAX_DIST);
			p.velocityContribs = p.velocity * f;
			p.velocityContribsSumWeights = f;
		}
		forEachNeighbourPair([&](Particle& p1, Particle& p2, vec2 const& vec, float dist) {
			float f = smoothKernel(dist, MAX_DIST);
			p1.velocityContribs += p2.velocity * f;
			p2.velocityContribs += p1.velocity * f;
			p1.velocityContribsSumWeights += f;
			p2.velocityContribsSumWeights += f;
			});
		for (auto& p : particles) {
			p.velocity = p.velocityContribs / p.velocityContribsSumWeights;
		}
	}


	void doFluidStep() {
		const auto maxX = sz.x;
		const auto maxY = sz.y;
		lxComputeForces();
		const float dampening = 0.75;
		for (auto& p : particles) {
			if (p.pos.x < 0 || p.pos.x > maxX) {
				p.velocity.x *= -1;
				p.velocity.x *= dampening;
			}
			if (p.pos.y < 0 || p.pos.y > maxY) {
				p.velocity.y *= -1;
				p.velocity.y *= dampening;
			}
			if (p.pos.x < 0) {
				p.pos.x = -p.pos.x;
			}
			if (p.pos.x > maxX) {
				p.pos.x = maxX - (p.pos.x - maxX);
			}
			if (p.pos.y < 0) {
				p.pos.y = -p.pos.y;
			}
			if (p.pos.y > maxY) {
				p.pos.y = maxY - (p.pos.y - maxY);
			}
		}
		for (auto& p : particles) {
			p.velocity += p.force + vec2(0, 0.07);
			p.pos += p.velocity;
			p.force = vec2(0, 0);
		}
	}
};
