module;
#include "precompiled.h"

export module lxlib.util;
/*
#ifdef FFTW3_H
			fftwf_free(arrayPtr);
#else
			delete[] arrayPtr;
#endif
*/

export enum nofill {};

export typedef glm::tvec3<unsigned char> bytevec3;

export template<class T>
struct Array2D
{
private:
	std::shared_ptr<T[]> dataSharedPtr;
public:
	T* data() { return dataSharedPtr.get(); }
	T const* data() const { return dataSharedPtr.get(); }
	typedef T value_type;
	int area;
	int w, h;
	int NumBytes() const {
		return area * sizeof(T);
	}
	ivec2 Size() const { return ivec2(w, h); }
	
	Array2D(int w, int h, nofill) {
		Init(w, h);
	}
	Array2D(ivec2 s, nofill) {
		Init(s.x, s.y);
	}
	Array2D(int w, int h, T const& defaultValue = T()) {
		Init(w, h);
		fill(defaultValue);
	}
	Array2D(ivec2 s, T const& defaultValue = T()) {
		Init(s.x, s.y);
		fill(defaultValue);
	}
	Array2D() {
		Init(0, 0);
	}

#ifdef OPENCV_CORE_HPP
	template<class TSrc>
	Array2D(cv::Mat_<TSrc> const& mat) : deleter(nullptr)
	{
		Init(mat.cols, mat.rows, (T*)mat.data);
	}
#endif

	T* begin() { return data(); }
	T* end() { return data()+w*h; }
	T const* begin() const { return data(); }
	T const* end() const { return data()+w*h; }
	
	T& operator()(int x, int y) { return data()[offsetOf(x, y)]; }
	T const& operator()(int x, int y) const { return data[offsetOf(x, y)]; }

	T& operator()(ivec2 const& v) { return data()[offsetOf(v.x, v.y)]; }
	T const& operator()(ivec2 const& v) const { return data()[offsetOf(v.x, v.y)]; }
	
	ivec2 wrapPoint(ivec2 p)
	{
		ivec2 wp = p;
		wp.x %= w; if(wp.x < 0) wp.x += w;
		wp.y %= h; if(wp.y < 0) wp.y += h;
		return wp;
	}
	
	T& wr(int x, int y) { return wr(ivec2(x, y)); }
	T const& wr(int x, int y) const { return wr(ivec2(x, y)); }

	T& wr(ivec2 const& v) { return (*this)(wrapPoint(v)); }
	T const& wr(ivec2 const& v) const { return (*this)(wrapPoint(v)); }

	bool contains(ivec2 const& p) const { return p.x >= 0 && p.y >= 0 && p.x < w && p.y < h; }

	Array2D clone() const {
		Array2D result(Size());
		std::copy(begin(), end(), result.begin());
		return result;
	}

private:
	int offsetOf(int x, int y) const { return y * w + x; }
	void fill(T const& value)
	{
		std::fill(begin(), end(), value);
	}
	void Init(int w, int h) {
#ifdef FFTW3_H
		auto dataPtr = (T*)fftwf_malloc(w * h * sizeof(T));
#else
		auto dataPtr = new T[w * h];
#endif
		Init(w, h, dataPtr);
	}
	void Init(int w, int h, T* dataPtr) {
		this->dataSharedPtr.reset(dataPtr);
		area = w * h;
		this->w = w;
		this->h = h;
	}
};

export const float pi = 3.14159265f;

// --- Implementations from util.cpp ---

export void rotate(vec2& p, float angle)
{
	float c = cos(angle), s = sin(angle);
	p = vec2(p.x * c + p.y * (-s), p.x * s + p.y * c);
}

export float randFloat()
{
	return rand() / (float)RAND_MAX;
}
