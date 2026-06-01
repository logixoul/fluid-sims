module;
#include "../precompiled.h"
#include <atomic>
#include <cmath>
//#include <numeric>

export module MultiscaleGrowthSketch;

import lxlib.Array2D;
import lxlib.stuff;
import lxlib.Array2D_imageProc;
import lxlib.gpgpu;
import lxlib.ConfigManager3;
import lxlib.Rect;
import lxlib.SketchBase;
import lxlib.shade;
import lxlib.TextureRef;
import lxlib.gpuBlur;
import lxlib.AudioSystem;
import ThisSketch_ImageProcessingHelpers;
import gpuBlurClaude;

int wsx = 700, wsy = 700;

using namespace ThisSketch;

lx::Array2D<float> img(256, 256);

export struct MultiscaleGrowthSketch : public lx::SketchBase {
	struct Options {
		float morphogenesisStrength;
		const float contrastizeStrength = 1.0f;
		float blendWeaken;
		float weightFactor;
		bool multiscale;
		bool doPostprocessing;
		float highPassStrength;
		lx::ConfigManager3 cfg;

		Options() : cfg("multiscaleGrowthConfig.toml") {}

		void init() {
			cfg.init();
		}

		void update() {
			morphogenesisStrength = cfg.getFloat("morphogenesisStrength");
			blendWeaken = cfg.getFloat("blendWeaken");
			weightFactor = cfg.getFloat("weightFactor");
			multiscale = cfg.getBool("multiscale");
			doPostprocessing = cfg.getBool("doPostprocessing");
			highPassStrength = cfg.getFloat("highPassStrength");
		}
	};

	Options options;
	bool isPaused = false;
	lx::AudioSystem audioSystem;

	std::atomic<float> micState = 0.0f;
	void audioCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer)
	{
		const float* samples = (const float*)inputBuffer;

		float peak = micState.load(std::memory_order_relaxed);
		for (int i = 0; i < framesPerBuffer; i++) {
			peak = std::max(peak, std::fabs(samples[i]));
		}
		micState.store(peak * 0.99f, std::memory_order_relaxed);
	}


	void setup()
	{
		options.init();
		reset();

		audioSystem.setupMic([this](const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer) {
			audioCallback(inputBuffer, outputBuffer, framesPerBuffer);
		});
	}

	~MultiscaleGrowthSketch()
	{
		audioSystem.shutdownMic();
	}

	void keyDown(int key)
	{
		if (keys['p']) {
			isPaused = !isPaused;
		}
		if (keys['r'])
		{
			reset();
		}
	}
	void reset() {
        for(auto p : img.coords()) {
         img(p) = lx::randFloat();
		}
	}
	lx::Array2D<float> updateSingleScale(lx::Array2D<float> aImg)
	{
		auto img = aImg.clone();

		auto tex = lx::uploadTex(img);
		auto gradsTex = lx::getGradients(tex, GL_CLAMP_TO_EDGE);

		tex->setWrap(GL_CLAMP_TO_EDGE);
		tex->sendParamsToGPU();
		gradsTex->setWrap(GL_CLAMP_TO_EDGE);
		gradsTex->sendParamsToGPU();

		auto tex2 = lx::shade({ tex, gradsTex }, R"(
			float f = lxTexture().x;
			_out.r = f;
			vec2 grad = lxTexture(tex1).xy;
			if(length(grad) == 0.0) {
				return;
			}
			vec2 gradN = normalize(grad);
			vec2 gradNPerp = vec2(-gradN.y, gradN.x);
			float atLeft = texture(tex0, texCoord - gradNPerp * texelSize0).x;
			float atRight = texture(tex0, texCoord + gradNPerp * texelSize0).x;
			float add = f - (atLeft + atRight) * 0.5;
			_out.r += add * morphogenesisStrength;
		)",
			lx::ShadeOpts().uniform("morphogenesisStrength", options.morphogenesisStrength));
		tex2 = lx::gaussianBlur3x3(tex2, GL_CLAMP_TO_EDGE);
		img = lx::downloadTex<float>(tex2);
		img = applyVerticalGradient(img);

		return img;
	}
	Img applyVerticalGradient(Img const& img) {
		Img result = lx::uninitializedArrayLike(img);
     for(auto p : result.coords()) {
           float floatY = p.y / (float)result.height();
			floatY = glm::mix(options.blendWeaken, 1.0f - options.blendWeaken, floatY);
			result(p) = blendHardLight(img(p), floatY);
		}
		return result;
	}
	float getLevelWeight(int level, int maxLevel) const {
		float iNormalized = level / float(maxLevel - 1);
		return exp(options.weightFactor * iNormalized);
	}
	std::vector<float> getLevelWeights(int numLevels) const {
		std::vector<float> result;
		for (int i = 0; i < numLevels; i++) {
			result.push_back(getLevelWeight(i, numLevels));
		}
		float sum = std::accumulate(result.begin(), result.end(), 0.0f);
		for (auto& weight : result) {
			//	weight /= sum;
		}
		return result;
	}
	Img multiscaleApply(Img src, function<Img(Img)> func) {
		std::vector<Img> origScales = ThisSketch::buildGaussianPyramid(src, 0.5f);
		std::vector<Img> updatedScales(origScales.size());
		const int last = origScales.size() - 1;
		updatedScales[last] = func(origScales[last]);
		auto weights = getLevelWeights(origScales.size());
		for (int i = updatedScales.size() - 1; i >= 1; i--) {
			auto diff = updatedScales[i] - origScales[i];
			diff = diff * weights[i];
			auto const upscaledDiff = gpuBlurClaude::singleblurLikeCinder(diff, origScales[i - 1].size());
			auto& nextScale = updatedScales[i - 1];
			nextScale = origScales[i - 1] + upscaledDiff;
			nextScale = func(nextScale);
		}
		return updatedScales[0];
	}
	void update() {
		lx::Array2D<float> newImg;
		if (options.multiscale)
			newImg = multiscaleApply(img, [this](auto arg) { return updateSingleScale(arg); });
		else
			newImg = updateSingleScale(img);
		if(!isPaused)
			img = newImg;
		img = lx::to01(img);

		//testMatchingFunctionality();
	}

	

	static lx::gl::TextureRef gpuHighpass(lx::gl::TextureRef in, float strength) {
		auto blurred = lx::gpuBlur::run(in, 2);
		auto highpassed = lx::shade({ in, blurred }, R"(
			float f = lxTexture().x;
			float fBlurred = lxTexture(tex1).x;
			float highPassed = f - fBlurred * highPassStrength;
			_out.r = highPassed;
			)", lx::ShadeOpts().uniform("highPassStrength", strength)
			);
		return highpassed;
	}
	lx::gl::TextureRef postprocess() {
		auto imgTex = lx::uploadTex(img);
		auto imgTexCentered = lx::shade(imgTex, R"(
			float f = lxTexture().x;
			_out.r = f - .5;
			)"
		);

		auto imgTexHighpassed = gpuHighpass(imgTexCentered, options.highPassStrength);
		
		auto result = lx::shade(imgTexHighpassed, R"(
			float f = lxTexture().x;
			float fw = fwidth(f);
			vec3 sum = vec3(0.0);


			float f1 = smoothstep(-fw/2.0, fw/2.0, f) - smoothstep(.01-fw/2.0, .01+fw/2.0, f);
			sum += f1 * vec3(1.0, 0.1, micInfluence*micInfluence);
			_out.rgb = sum;
			)", lx::ShadeOpts()
					.dstRectSize(ivec2(wsx, wsy))
					.ifmt(GL_RGBA16F)
					.uniform("micInfluence", options.blendWeaken)
		);	
		
		auto resultB = gpuBlurClaude::blurWithInvKernel(result);
		result = op(result) + op(resultB) * 3.0;

		result = lx::shade({ result, imgTex }, R"(
			vec3 bloomedHiPass = lxTexture(tex0).rgb;
			vec3 original = vec3(lxTexture(tex1).r);
			vec3 sum = bloomedHiPass * 4.0;
			sum /= sum + vec3(1.0);
			_out.rgb = sum;
		)");

		return result;
	}
	
	float micInfluence;
	void draw()
	{
		lx::lxClear();
		options.update();

		micInfluence = micState.load(std::memory_order_relaxed);
		micInfluence *= 4;
		micInfluence = std::max(0.0, 1.0 - micInfluence*micInfluence);
		std::cout << micInfluence << std::endl;
		options.blendWeaken = micInfluence;

        lx::gl::TextureRef tex = lx::uploadTex(img);
		if (options.doPostprocessing) {
			tex = postprocess();
		}
		else {
			tex = redToLuminance(tex);
		}
		glViewport(0, 0, windowSize.x, windowSize.y);
		lx::lxDraw(tex, lx::Rect<float>(0, 0, 1, 1));
	}
};

export using StartupSketch = MultiscaleGrowthSketch;

