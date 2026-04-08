module;
#include "precompiled.h"
#include "macros.h"

export module lxlib.shade;

import lxlib.GlslProg;
import lxlib.TextureRef;
import lxlib.Array2D;
import lxlib.stuff;
import lxlib.VaoVbo;

export struct GpuScope {
	GpuScope(string name);
	~GpuScope();
};

export void drawRect();

export struct Str {
	string s;
	Str& operator<<(string s2) {
		s += s2 + "\n";
		return *this;
	}
	Str& operator<<(Str s2) {
		s += s2.s + "\n";
		return *this;
	}
	operator std::string() {
		return s;
	}
};

export struct Uniform {
	function<void(GlslProgRef)> setter;
	string shortDecl;
};

export template<class T>
struct optional {
	T val;
	bool exists;
	optional(T const& t) { val=t; exists=true; }
	optional() { exists=false; }
};

export template<class T> string typeToString();
export template<> inline string typeToString<float>() {
	return "float";
}
export template<> inline string typeToString<int>() {
	return "int";
}
export template<> inline string typeToString<ivec2>() {
	return "ivec2";
}
export template<> inline string typeToString<vec2>() {
	return "vec2";
}

export struct ShadeOpts
{
	ShadeOpts();
	ShadeOpts& ifmt(GLenum val) { _ifmt=val; return *this; }
	ShadeOpts& scale(float val) { _scaleX=val; _scaleY=val; return *this; }
	ShadeOpts& scale(float valX, float valY) { _scaleX=valX; _scaleY=valY; return *this; }
	ShadeOpts& scope(std::string name) { _scopeName = name; return *this; }
	ShadeOpts& targetTex(gl::TextureRef val) { _targetTexs = { val }; return *this; }
	ShadeOpts& targetTexs(vector<gl::TextureRef> val) { _targetTexs = val; return *this; }
	ShadeOpts& targetImg(gl::TextureRef val) { _targetImg = val; return *this; }
	ShadeOpts& dstPos(ivec2 val) { _dstPos = val; return *this; }
	ShadeOpts& dstRectSize(ivec2 val) { _dstRectSize = val; return *this; }
	ShadeOpts& enableResult(bool val) {
		_enableResult = val; return *this;
	}
	template<class T>
	ShadeOpts& uniform(string name, T val) {
		_uniforms.push_back(Uniform{
			[val, name](GlslProgRef prog) { prog->uniform(name, val); },
			typeToString<T>() + " " + name
			});
		return *this;
	}
	ShadeOpts& vshaderExtra(string val) {
		_vshaderExtra = val;
		return *this;
	}
	ShadeOpts& functions(string val) {
		_functions = val;
		return *this;
	}

	optional<GLenum> _ifmt;
	float _scaleX, _scaleY;
	std::string _scopeName;
	vector<gl::TextureRef> _targetTexs;
	gl::TextureRef _targetImg = nullptr;
	ivec2 _dstPos;
	ivec2 _dstRectSize = ivec2(0, 0);
	bool _enableResult = true;
	vector<Uniform> _uniforms;
	string _vshaderExtra;
	std::string _functions;
};

void drawRect() {
	static std::shared_ptr<QuadGpu> quad = createQuadVAO_VBOs();
	quad->vao.bind();
	glDrawArrays(GL_TRIANGLES, 0, 6);
	VAO::unbind();
}

auto samplerSuffix = [&](int i) -> string {
	return (i == 0) ? "" : std::to_string(1 + i);
};

auto samplerName = [&](int i) -> string {
	return "tex" + samplerSuffix(i);
};

std::string getCompleteFshader(vector<gl::TextureRef> const& texv, vector<Uniform> const& uniforms, std::string const& fshader, std::string const& functions, string* uniformDeclarationsRet) {
	auto texIndex = [&](gl::TextureRef t) {
		return std::to_string(
			1 + (std::find(texv.begin(), texv.end(), t) - texv.begin())
		);
		};
	stringstream uniformDeclarations;
	int location = 0;
	uniformDeclarations << "uniform ivec2 viewportSize;\n";
	uniformDeclarations << "uniform vec2 mouse;\n";
	for (int i = 0; i < texv.size(); i++)
	{
		string samplerType = "sampler2D";
		uniformDeclarations << "uniform " + samplerType + " " + samplerName(i) + ";\n";
		uniformDeclarations << "uniform vec2 " + samplerName(i) + "Size;\n";
		uniformDeclarations << "uniform vec2 tsize" + samplerSuffix(i) + ";\n";
	}
	for (auto& p : uniforms)
	{
		uniformDeclarations << "uniform " + p.shortDecl + ";\n";
	}
	*uniformDeclarationsRet = uniformDeclarations.str();
	string const fullText =
		Str()
		<< "#version 150"
		<< "#extension GL_ARB_explicit_uniform_location : enable"
		<< "#extension GL_ARB_texture_gather : enable"
		<< uniformDeclarations.str()
        << "in vec2 texCoord;"
		<< "out vec4 _out;"
		<< "vec4 texture(sampler2D tex_) {"
      << "	return texture(tex_, texCoord);"
		<< "}"
		<< "vec4 texture() {"
       << "	return texture(tex, texCoord);"
		<< "}"
		<< functions
		<< "void main() {"
		<< "#line 0\n\n" // the \n\n is needed only on Intel gpus. Probably a driver bug.
		<< fshader
		<< "}"
		;
	return fullText;
}

export gl::TextureRef shade(vector<gl::TextureRef> const& texv, std::string const& fshader, ShadeOpts const& opts = ShadeOpts())
{
	shared_ptr<GpuScope> gpuScope;
	if (opts._scopeName != "") {
		gpuScope = make_shared<GpuScope>(opts._scopeName);
	}
	static std::map<string, GlslProgRef> shaders;
	GlslProgRef shader;
	if(shaders.find(fshader) == shaders.end())
	{
		string uniformDeclarations;
		std::string completeFshader = getCompleteFshader(texv, opts._uniforms, fshader, opts._functions, &uniformDeclarations);
		string completeVshader = Str()
			<< "#version 150"
			<< "#extension GL_ARB_explicit_attrib_location : enable"
			//<< "#extension GL_ARB_shader_image_load_store : enable"
			<< "layout(location = 0) in vec4 ciPosition;"
			<< "layout(location = 1) in vec2 ciTexCoord0;"
         << "out highp vec2 texCoord;"
			<< uniformDeclarations

			<< "void main()"
			<< "{"
			<< "	gl_Position = ciPosition * 2 - 1;"
          << "	texCoord = ciTexCoord0;"
			<< opts._vshaderExtra
			<< "}";
		try{
			shader = std::make_shared<GlslProg>(completeFshader, completeVshader);
			shaders[fshader] = shader;
		} catch(std::runtime_error const& e) {
			cout << "GlslProgCompileExc: " << e.what() << endl;
			cout << "source:" << endl;
			cout << completeFshader << endl;
			string s; cin >> s;
			throw;
		}
	} else {
		shader = shaders[fshader];
	}
	auto tex0 = texv[0];
	shader->bind();
	ivec2 viewportSize(
		floor(tex0->getWidth() * opts._scaleX),
		floor(tex0->getHeight() * opts._scaleY)
	);

	if (opts._dstRectSize != ivec2(0, 0)) {
		viewportSize = opts._dstRectSize;
	}
	vector<gl::TextureRef> results;
	if (opts._enableResult) {
		if (opts._targetTexs.size() != 0)
		{
			results = opts._targetTexs;
		}
		else {
			GLenum ifmt = opts._ifmt.exists ? opts._ifmt.val : tex0->getInternalFormat();
			results = { getTexFromPool(viewportSize.x, viewportSize.y, ifmt) };
		}
	}
	
	int location = 0;
	shader->uniform("viewportSize", viewportSize);
	location++;
	shader->uniform("tex", 0); tex0->bind(GL_TEXTURE0 + 0);
	shader->uniform("texSize", vec2(tex0->getSize()));
	shader->uniform("tsize", vec2(1.0) / vec2(tex0->getSize()));
	for (int i = 1; i < texv.size(); i++) {
		shader->uniform(samplerName(i), i); texv[i]->bind(GL_TEXTURE0 + i);
		shader->uniform(samplerName(i)+"Size", vec2(texv[i]->getSize()));
		shader->uniform("tsize" + samplerSuffix(i), vec2(1)/vec2(texv[i]->getSize()));
	}
	for (auto& uniform : opts._uniforms)
	{
		uniform.setter(shader);
	}
	
	shader->bind();

	if (opts._enableResult) {
		beginRTT(results);
	}
	else {
		// if we don't do that, OpenGL clamps the viewport (that we set) by the cinder window size
		beginRTT(opts._targetImg);

		glColorMask(false, false, false, false);
	}
	
	{
		glViewport(0, 0, viewportSize.x, viewportSize.y);
		glDisable(GL_BLEND);
		::drawRect();
	}
	if (opts._enableResult) {
		endRTT();
	}
	else {
		endRTT();
		glColorMask(true, true, true, true);
	}
	return results[0];
}

export gl::TextureRef shade(gl::TextureRef const& tex, std::string const& fshader, ShadeOpts const& opts = ShadeOpts()) {
	return shade(std::vector<gl::TextureRef>{ tex }, fshader, opts);
}

GpuScope::GpuScope(string name) {
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
}

GpuScope::~GpuScope() {
	glPopDebugGroup();
}

ShadeOpts::ShadeOpts() {
	//_ifmt=GL_RGBA16F;
	_scaleX = _scaleY = 1.0f;
}
