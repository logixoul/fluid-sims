module;
#include "precompiled.h"

export module lxlib.TextureCache;

import lxlib.TextureRef;

export namespace lx {
	struct TextureCacheKey {
		glm::ivec2 size;
		GLenum ifmt;
		bool allocateMipmaps = false;

		bool operator==(const lx::TextureCacheKey &other) const
		{
			return size == other.size
				&& ifmt == other.ifmt
				&& allocateMipmaps == other.allocateMipmaps
				;
		}
	};

	struct TextureCacheKeyHasher {
		std::size_t operator()(lx::TextureCacheKey const& k) const
		{
			return k.size.x ^ k.size.y ^ k.ifmt ^ k.allocateMipmaps;
		}
	};

	class TextureCache
	{
	public:
		static lx::TextureCache* instance();
		lx::gl::TextureRef get(lx::TextureCacheKey const& key);
		static void clearCaches();

		static void printTextures();

		static void deleteTexture(lx::gl::TextureRef tex);

	private:
		TextureCache();

     std::unordered_map<lx::TextureCacheKey, std::vector<lx::gl::TextureRef>, lx::TextureCacheKeyHasher> cache;
	};
}

// --- Implementations from TextureCache.cpp ---

int count1;
int count2;

lx::TextureCache* lx::TextureCache::instance() {
	static lx::TextureCache obj;
	return &obj;
}

std::map<int, std::string> fmtMap = {
	{ GL_R16F, "GL_R16F" },
	{ GL_RGB16F, "GL_RGB16F" },
	{ GL_RG32F, "GL_RG32F" },
	{ GL_R32F, "GL_R32F" },
	{ GL_RGBA16F, "GL_RGBA16F" },
	{ GL_RGB8, "GL_RGB8" },
};

std::map<int, int> fmtMapBpp = {
	{ GL_R16F, 2 },
	{ GL_R32F, 4 },
	{ GL_RGB16F, 6 },
	{ GL_RG32F, 8 },
	{ GL_RGBA16F, 8 },
	{ GL_RGB8, 3 },
};

lx::TextureCache::TextureCache() {
}

static lx::gl::TextureRef allocTex(lx::TextureCacheKey const& key) {
	lx::gl::Texture::Format fmt;
	fmt.setInternalFormat(key.ifmt);
	fmt.setImmutableStorage(true);
	fmt.enableMipmapping(key.allocateMipmaps);
    auto tex = lx::gl::Texture::create(key.size.x, key.size.y, fmt);
	return tex;
}

void setDefaults(lx::gl::TextureRef tex) {
	tex->setMinFilter(GL_LINEAR);
	tex->setMagFilter(GL_LINEAR);
	tex->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
}

lx::gl::TextureRef lx::TextureCache::get(lx::TextureCacheKey const & key)
{
	auto it = cache.find(key);
	if (it == cache.end()) {
		auto tex = allocTex(key);
     std::vector<lx::gl::TextureRef> vec{ tex };
		cache.insert(std::make_pair(key, vec));
		setDefaults(tex);
		return tex;
	}
	else {
		auto& vec = it->second;
		for (auto& texRef : vec) {
			if (texRef.use_count() == 1) {
				count2++;
				setDefaults(texRef);
				return texRef;
			}
		}
		count1++;
		if (key.ifmt == GL_R32F && key.size.x == 5312) {
			std::cout << "hey" << std::endl;
		}

		auto tex = allocTex(key);
		vec.push_back(tex);
		setDefaults(tex);
		return tex;
	}
}

void lx::TextureCache::clearCaches()
{
	auto& cache = instance()->cache;
	
	for (auto& pair : cache) {
      std::vector<lx::gl::TextureRef> remaining;
		for (auto& tex : pair.second) {
			if (tex.use_count() > 1) {
				remaining.push_back(tex);
			}
		}
		cache[pair.first] = remaining;
	}
}

void lx::TextureCache::printTextures()
{
	throw std::runtime_error("TextureCache::printTextures(): Not implemented");
}

void lx::TextureCache::deleteTexture(lx::gl::TextureRef texToDel)
{
	auto& cache = instance()->cache;

	for (auto& pair : cache) {
      std::vector<lx::gl::TextureRef> remaining;
		for (auto& tex : pair.second) {
			if (tex != texToDel) {
				remaining.push_back(tex);
			}
		}
		cache[pair.first] = remaining;
	}
}
