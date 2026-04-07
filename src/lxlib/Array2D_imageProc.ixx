module;
#include "precompiled.h"
#include "macros.h"

export module lxlib.Array2D_imageProc;

import lxlib.Array2D;
import lxlib.stuff;

export namespace WrapModes {
	struct GetMirrorWrapped {
		template<class T>
		static T& fetch(Array2D<T>& src, int x, int y)
		{
			if (x >= src.w) x = src.w - (x - src.w) - 1;
			else if (x < 0) x = -x;
			if (y >= src.h) y = src.h - (y - src.h) - 1;
			else if (y < 0) y = -y;
			return src(x, y);
		}
	};
	struct GetWrapped {
		template<class T>
		static T& fetch(Array2D<T>& src, int x, int y)
		{
			return src.wr(x, y);
		}
	};
	struct Get_WrapZeros {
		template<class T>
		static T& fetch(Array2D<T>& src, int x, int y)
		{
			if (x < 0 || y < 0 || x >= src.w || y >= src.h)
			{
				static T zeroRef = ::zero<T>();
				return zeroRef;
			}
			return src(x, y);
		}
	};
	struct NoWrap {
		template<class T>
		static T& fetch(Array2D<T>& src, int x, int y)
		{
			return src(x, y);
		}
	};
	struct GetClamped {
		template<class T>
		static T& fetch(Array2D<T>& src, int x, int y)
		{
			if (x < 0) x = 0;
			if (x > src.w - 1) x = src.w - 1;
			if (y < 0) y = 0;
			if (y > src.h - 1) y = src.h - 1;
			
			return src(x, y);
		}
	};
	typedef GetWrapped DefaultImpl;
};
export template<class T, class FetchFunc>
void splatBilinearPoint(Array2D<T>& dst, glm::vec2 const& p, T const& value)
{
	int ix = p.x, iy = p.y;
	float fx = ix, fy = iy;
	if (p.x < 0.0f && fx != p.x) { fx--; ix--; }
	if (p.y < 0.0f && fy != p.y) { fy--; iy--; }
	float fractx = p.x - fx;
	float fracty = p.y - fy;
	float fractx1 = 1.0 - fractx;
	float fracty1 = 1.0 - fracty;
	FetchFunc::fetch(dst, ix, iy) += (fractx1 * fracty1) * value;
	FetchFunc::fetch(dst, ix, iy + 1) += (fractx1 * fracty) * value;
	FetchFunc::fetch(dst, ix + 1, iy) += (fractx * fracty1) * value;
	FetchFunc::fetch(dst, ix + 1, iy + 1) += (fractx * fracty) * value;
}
export template<class T, class FetchFunc>
T getBilinear(Array2D<T> const& src, glm::vec2 const& p)
{
	int ix = p.x, iy = p.y;
	float fx = ix, fy = iy;
	if (p.x < 0.0f && fx != p.x) { fx--; ix--; }
	if (p.y < 0.0f && fy != p.y) { fy--; iy--; }
	float fractx = p.x - fx;
	float fracty = p.y - fy;
	return lerp(
		lerp(FetchFunc::fetch(src, ix, iy), FetchFunc::fetch(src, ix + 1, iy), fractx),
		lerp(FetchFunc::fetch(src, ix, iy + 1), FetchFunc::fetch(src, ix + 1, iy + 1), fractx),
		fracty);
}

export template<class T>
T& zero() {
	static T val = T()*0.0f;
	val = T(0.0f);
	return val;
}

export template<class T>
Array2D<T> gauss3(Array2D<T> src) {
	T zero = ::zero<T>();
	Array2D<T> dst1(src.w, src.h);
	Array2D<T> dst2(src.w, src.h);
	forxy(dst1)
		dst1(p) = .25f * (2.0f * get_clamped(src, p.x, p.y) + get_clamped(src, p.x - 1, p.y) + get_clamped(src, p.x + 1, p.y));
	forxy(dst2)
		dst2(p) = .25f * (2.0f * get_clamped(dst1, p.x, p.y) + get_clamped(dst1, p.x, p.y - 1) + get_clamped(dst1, p.x, p.y + 1));
	return dst2;
}

export template<class T, class FetchFunc = WrapModes::DefaultImpl>
vec2 gradient_i(Array2D<T>& src, ivec2 const& p)
{
	vec2 gradient;
	gradient.x = (FetchFunc::fetch(src, p.x + 1, p.y) - FetchFunc::fetch(src, p.x - 1, p.y)) / 2.0f;
	gradient.y = (FetchFunc::fetch(src, p.x, p.y + 1) - FetchFunc::fetch(src, p.x, p.y - 1)) / 2.0f;
	return gradient;
}
template<class T, class FetchFunc>
vec2 gradient_i_nodiv(Array2D<T>& src, ivec2 const& p)
{
	vec2 gradient(
		FetchFunc::fetch(src, p.x + 1, p.y) - FetchFunc::fetch(src, p.x - 1, p.y),
		FetchFunc::fetch(src, p.x, p.y + 1) - FetchFunc::fetch(src, p.x, p.y - 1));
	return gradient;
}
export template<class T, class FetchFunc>
Array2D<vec2> get_gradients(Array2D<T>& src)
{
	auto src2 = src.clone();
	forxy(src2)
		src2(p) /= 2.0f;
	Array2D<vec2> gradients(src.w, src.h);
	for (int x = 0; x < src.w; x++)
	{
		gradients(x, 0) = gradient_i_nodiv<T, FetchFunc>(src2, ivec2(x, 0));
		gradients(x, src.h - 1) = gradient_i_nodiv<T, FetchFunc>(src2, ivec2(x, src.h - 1));
	}
	for (int y = 1; y < src.h - 1; y++)
	{
		gradients(0, y) = gradient_i_nodiv<T, FetchFunc>(src2, ivec2(0, y));
		gradients(src.w - 1, y) = gradient_i_nodiv<T, FetchFunc>(src2, ivec2(src.w - 1, y));
	}
	for (int y = 1; y < src.h - 1; y++) {
		for (int x = 1; x < src.w - 1; x++) {
			gradients(x, y) = gradient_i_nodiv<T, WrapModes::NoWrap>(src2, ivec2(x, y));
		}
	}
	return gradients;
}

export template<class T, class FetchFunc>
Array2D<T> separableConvolve(Array2D<T> const& src, vector<float> const& kernel) {
	int ksize = kernel.size();
	int r = ksize / 2;

	T zero = ::zero<T>();
	Array2D<T> dst1(src.w, src.h);
	Array2D<T> dst2(src.w, src.h);

	int w = src.w, h = src.h;

	for (int y = 0; y < h; y++)
	{
		auto blurVert = [&](int x0, int x1) {
			x0 = std::max(x0, 0);
			x1 = std::min(x1, w);

			for (int x = x0; x < x1; x++)
			{
				T sum = zero;
				for (int xadd = -r; xadd <= r; xadd++)
				{
					sum += kernel[xadd + r] * (FetchFunc::fetch(src, x + xadd, y));
				}
				dst1(x, y) = sum;
			}
		};

		blurVert(0, r);
		blurVert(w - r, w);
		for (int x = r; x < w - r; x++)
		{
			T sum = zero;
			for (int xadd = -r; xadd <= r; xadd++)
			{
				sum += kernel[xadd + r] * src(x + xadd, y);
			}
			dst1(x, y) = sum;
		}
	}

	for (int x = 0; x < w; x++)
	{
		auto blurHorz = [&](int y0, int y1) {
			y0 = std::max(y0, 0);
			y1 = std::min(y1, h);
			for (int y = y0; y < y1; y++)
			{
				T sum = zero;
				for (int yadd = -r; yadd <= r; yadd++)
				{
					sum += kernel[yadd + r] * FetchFunc::fetch(dst1, x, y + yadd);
				}
				dst2(x, y) = sum;
			}
		};

		blurHorz(0, r);
		blurHorz(h - r, h);
		for (int y = r; y < h - r; y++)
		{
			T sum = zero;
			for (int yadd = -r; yadd <= r; yadd++)
			{
				sum += kernel[yadd + r] * dst1(x, y + yadd);
			}
			dst2(x, y) = sum;
		}
	}
	return dst2;
}

export template<class T, class FetchFunc>
Array2D<T> gaussianBlur(Array2D<T> src, int ksize) {
	auto kernel = getGaussianKernel(ksize, sigmaFromKsize(ksize));
	return separableConvolve<T, FetchFunc>(src, kernel);
}

// --- Implementations from Array2D_imageProc.cpp ---

export void mm(string desc, Array2D<float> arr) {
	if (desc != "") {
		cout << "[" << desc << "] ";
	}
	cout << "min: " << *std::min_element(arr.begin(), arr.end()) << ", "
		<< "max: " << *std::max_element(arr.begin(), arr.end()) << endl;
}
export void mm(string desc, Array2D<vec3> arr) {
	if (desc != "") {
		cout << "[" << desc << "] ";
	}
	auto data = (float*)arr.data();
	cout << "min: " << *std::min_element(data, data + arr.area * 3) << ", "
		<< "max: " << *std::max_element(data, data + arr.area * 3) << endl;
}
export void mm(string desc, Array2D<vec2> arr) {
	if (desc != "") {
		cout << "[" << desc << "] ";
	}
	auto data = (float*)arr.data();
	cout << "min: " << *std::min_element(data, data + arr.area * 2) << ", "
		<< "max: " << *std::max_element(data, data + arr.area * 2) << endl;
}

export vector<float> getGaussianKernel(int ksize, float sigma) {
	vector<float> result;
	int r = ksize / 2;
	float sum = 0.0f;
	for (int i = -r; i <= r; i++) {
		float exponent = -(i*i / sq(2 * sigma));
		float val = exp(exponent);
		sum += val;
		result.push_back(val);
	}
	for (int i = 0; i < result.size(); i++) {
		result[i] /= sum;
	}
	return result;
}

// This is a common heuristic for choosing sigma based on kernel size, used in
// OpenCV and elsewhere.
export float sigmaFromKsize(float ksize) {
	float sigma = 0.3*((ksize - 1)*0.5 - 1) + 0.8;
	return sigma;
}

// This is the inverse of the above heuristic, which is useful for choosing
// kernel size based on a desired sigma.
export float ksizeFromSigma(float sigma) {
	int ksize = ceil(((sigma - 0.8) / 0.3 + 1) / 0.5 + 1);
	if (ksize % 2 == 0)
		ksize++;
	return ksize;
}

// Linearly remaps the values in the array to be between 0 and 1, based
// on the min and max values in the array.
export Array2D<float> to01(Array2D<float> a) {
	auto minn = *std::min_element(a.begin(), a.end());
	auto maxx = *std::max_element(a.begin(), a.end());
	auto b = a.clone();
	forxy(b) {
		b(p) -= minn;
		b(p) /= (maxx - minn);
	}
	return b;
}

