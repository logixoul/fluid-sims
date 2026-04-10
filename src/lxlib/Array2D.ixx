module;
#include "precompiled.h"
#include "macros.h"

export module lxlib.Array2D;
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
       struct CoordIterator {
			ivec2 p;
			int width;

			ivec2 operator*() const { return p; }

			CoordIterator& operator++() {
				++p.x;
				if(p.x >= width) {
					p.x = 0;
					++p.y;
				}
				return *this;
			}

			bool operator!=(CoordIterator const& other) const {
				return p.x != other.p.x || p.y != other.p.y;
			}
		};

		struct CoordRange {
			int width;
			int height;

			CoordIterator begin() const {
				if(width <= 0 || height <= 0) {
					return end();
				}
				return CoordIterator{ ivec2(0, 0), width };
			}

			CoordIterator end() const {
				return CoordIterator{ ivec2(0, height), width };
			}
		};

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

	T* begin() { return data(); }
	T* end() { return data()+w*h; }
	T const* begin() const { return data(); }
	T const* end() const { return data()+w*h; }
		CoordRange coords() const { return CoordRange{ w, h }; }
	
	T& operator()(int x, int y) { return data()[offsetOf(x, y)]; }
	T const& operator()(int x, int y) const { return data()[offsetOf(x, y)]; }

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
		Array2D result(Size(), nofill());
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
     if constexpr (std::is_trivially_default_constructible_v<T> && std::is_trivially_destructible_v<T>) {
			auto dataPtr = (T*)fftwf_malloc(w * h * sizeof(T));
			Init(w, h, dataPtr, [](T* ptr) {
				fftwf_free(ptr);
			});
			return;
		}
		auto dataPtr = new T[w * h];
#else
		auto dataPtr = new T[w * h];
#endif
		Init(w, h, dataPtr);
	}
	void Init(int w, int h, T* dataPtr) {
     Init(w, h, dataPtr, std::default_delete<T[]>());
	}
	template<class TDeleter>
	void Init(int w, int h, T* dataPtr, TDeleter deleter) {
		this->dataSharedPtr = std::shared_ptr<T[]>(dataPtr, deleter);
		area = w * h;
		this->w = w;
		this->h = h;
	}
};

export template<class T>
Array2D<T> uninitializedArrayLike(Array2D<T> a) {
	return Array2D<T>(a.Size(), nofill());
}

export template<class T>
Array2D<T> onesLike(Array2D<T> a) {
	return Array2D<T>(a.Size(), 1.0f);
}

export template<class T>
Array2D<T> zerosLike(Array2D<T> a) {
	return Array2D<T>(a.Size(), ::zero<T>());
}


template<class TTarget, class TValue>
TTarget broadcastTo(TValue const& value) {
	// this works because glm vector types have a constructor that takes a single scalar
	// and broadcasts it to all components, and for non-vector types this just returns
	// the value as-is
    return TTarget(value);
}

template<class T, class TOp>
Array2D<T>& applyOpInPlace(Array2D<T>& arr, T const& secondArg, TOp const& op)
{
    for(auto p : arr.coords()) {
		arr(p) = op(arr(p), secondArg);
	}
	return arr;
}

template<class T1, class T2, class TOp>
Array2D<T1>& applyArrayArrayOpInPlace(Array2D<T1>& arr1, Array2D<T2> const& arr2, TOp const& op)
{
    assert(arr1.w == arr2.w && arr1.h == arr2.h);
   for(auto p : arr1.coords()) {
		arr1(p) = op(arr1(p), broadcastTo<T1>(arr2(p)));
	}
 return arr1;
}

template<class T, class TOp>
Array2D<T> applyOpOutOfPlace(Array2D<T> const& arr, T const& secondArg, TOp const& op)
{
	auto resultArr = uninitializedArrayLike(arr);
    for(auto p : arr.coords()) {
		resultArr(p) = op(arr(p), secondArg);
	}
	return resultArr;
}

template<class T, class TOp>
Array2D<T> applyOpOutOfPlace(T const& firstArg, Array2D<T> const& arr, TOp const& op)
{
	auto resultArr = uninitializedArrayLike(arr);
    for(auto p : arr.coords()) {
		resultArr(p) = op(firstArg, arr(p));
	}
	return resultArr;
}

template<class T1, class T2, class TOp>
Array2D<T1> applyArrayArrayOpOutOfPlace(Array2D<T1> const& arr1, Array2D<T2> const& arr2, TOp const& op)
{
	assert(arr1.w == arr2.w && arr1.h == arr2.h);
	auto resultArr = uninitializedArrayLike(arr1);
   for(auto p : arr1.coords()) {
		resultArr(p) = op(arr1(p), broadcastTo<T1>(arr2(p)));
	}
	return resultArr;
}

// this allows both Array2D<float> + float and Array2D<glm::vec3> + glm::vec3, but also Array2D<glm::vec3> + float (which adds the float to each component of the vec3, a.k.a. "broadcasts" the float)
export template<class T, class TSecondArg>
Array2D<T> operator+(Array2D<T> const& arr, TSecondArg const& secondArg)
{
	return applyOpOutOfPlace(arr, broadcastTo<T>(secondArg), std::plus<T>());
}

export template<class T, class TSecondArg>
Array2D<T>& operator+=(Array2D<T>& arr, TSecondArg const& secondArg)
{
	return applyOpInPlace(arr, broadcastTo<T>(secondArg), std::plus<T>());
}

export template<class T, class TFirstArg>
Array2D<T> operator+(TFirstArg const& firstArg, Array2D<T> const& arr)
{
	return applyOpOutOfPlace(broadcastTo<T>(firstArg), arr, std::plus<T>());
}

export template<class T1, class T2>
Array2D<T1> operator+(Array2D<T1> const& arr1, Array2D<T2> const& arr2)
{
	return applyArrayArrayOpOutOfPlace(arr1, arr2, std::plus<T1>());
}

export template<class T1, class T2>
Array2D<T1>& operator+=(Array2D<T1>& arr1, Array2D<T2> const& arr2)
{
	return applyArrayArrayOpInPlace(arr1, arr2, std::plus<T1>());
}

export template<class T, class TSecondArg>
Array2D<T> operator-(Array2D<T> const& arr, TSecondArg const& secondArg)
{
	return applyOpOutOfPlace(arr, broadcastTo<T>(secondArg), std::minus<T>());
}

export template<class T, class TSecondArg>
Array2D<T>& operator-=(Array2D<T>& arr, TSecondArg const& secondArg)
{
	return applyOpInPlace(arr, broadcastTo<T>(secondArg), std::minus<T>());
}

export template<class T, class TFirstArg>
Array2D<T> operator-(TFirstArg const& firstArg, Array2D<T> const& arr)
{
	return applyOpOutOfPlace(broadcastTo<T>(firstArg), arr, std::minus<T>());
}

export template<class T1, class T2>
Array2D<T1> operator-(Array2D<T1> const& arr1, Array2D<T2> const& arr2)
{
	return applyArrayArrayOpOutOfPlace(arr1, arr2, std::minus<T1>());
}

export template<class T1, class T2>
Array2D<T1>& operator-=(Array2D<T1>& arr1, Array2D<T2> const& arr2)
{
	return applyArrayArrayOpInPlace(arr1, arr2, std::minus<T1>());
}

export template<class T, class TSecondArg>
Array2D<T> operator*(Array2D<T> const& arr, TSecondArg const& secondArg)
{
	return applyOpOutOfPlace(arr, broadcastTo<T>(secondArg), std::multiplies<T>());
}

export template<class T, class TSecondArg>
Array2D<T>& operator*=(Array2D<T>& arr, TSecondArg const& secondArg)
{
	return applyOpInPlace(arr, broadcastTo<T>(secondArg), std::multiplies<T>());
}

export template<class T, class TFirstArg>
Array2D<T> operator*(TFirstArg const& firstArg, Array2D<T> const& arr)
{
	return applyOpOutOfPlace(broadcastTo<T>(firstArg), arr, std::multiplies<T>());
}

export template<class T1, class T2>
Array2D<T1> operator*(Array2D<T1> const& arr1, Array2D<T2> const& arr2)
{
	return applyArrayArrayOpOutOfPlace(arr1, arr2, std::multiplies<T1>());
}

export template<class T1, class T2>
Array2D<T1>& operator*=(Array2D<T1>& arr1, Array2D<T2> const& arr2)
{
	return applyArrayArrayOpInPlace(arr1, arr2, std::multiplies<T1>());
}

export template<class T, class TSecondArg>
Array2D<T> operator/(Array2D<T> const& arr, TSecondArg const& secondArg)
{
	return applyOpOutOfPlace(arr, broadcastTo<T>(secondArg), std::divides<T>());
}

export template<class T, class TSecondArg>
Array2D<T>& operator/=(Array2D<T>& arr, TSecondArg const& secondArg)
{
	return applyOpInPlace(arr, broadcastTo<T>(secondArg), std::divides<T>());
}

export template<class T, class TFirstArg>
Array2D<T> operator/(TFirstArg const& firstArg, Array2D<T> const& arr)
{
	return applyOpOutOfPlace(broadcastTo<T>(firstArg), arr, std::divides<T>());
}

export template<class T1, class T2>
Array2D<T1> operator/(Array2D<T1> const& arr1, Array2D<T2> const& arr2)
{
	return applyArrayArrayOpOutOfPlace(arr1, arr2, std::divides<T1>());
}

export template<class T1, class T2>
Array2D<T1>& operator/=(Array2D<T1>& arr1, Array2D<T2> const& arr2)
{
	return applyArrayArrayOpInPlace(arr1, arr2, std::divides<T1>());
}



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

export float randFloat(float min, float max)
{
	return randFloat() * (max - min) + min;
}