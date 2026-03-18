module;

#include <cmath>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#ifdef _WIN32
#include <float.h>
#endif
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/io.hpp>

// forxy is private to this TU (cannot export macros from modules)
// Modules that need forxy must define it in their own global module fragment.
#define forxy(image) \
    for(glm::ivec2 p(0, 0); p.y < image.h; p.y++) \
        for(p.x = 0; p.x < image.w; p.x++)

export module util;

using namespace glm;
using namespace std;

// ---------- ArrayDeleter ----------

export template<class T>
class ArrayDeleter
{
public:
    ArrayDeleter(T* arrayPtr)
    {
        refcountPtr = new int(0);
        (*refcountPtr)++;
        this->arrayPtr = arrayPtr;
    }

    ArrayDeleter(ArrayDeleter const& other)
    {
        arrayPtr = other.arrayPtr;
        refcountPtr = other.refcountPtr;
        (*refcountPtr)++;
    }

    ArrayDeleter const& operator=(ArrayDeleter const& other)
    {
        reduceRefcount();
        arrayPtr = other.arrayPtr;
        refcountPtr = other.refcountPtr;
        (*refcountPtr)++;
        return *this;
    }

    ~ArrayDeleter()
    {
        reduceRefcount();
    }

private:
    void reduceRefcount()
    {
        (*refcountPtr)--;
        if (*refcountPtr == 0)
        {
            delete refcountPtr;
            delete[] arrayPtr;
        }
    }

    int* refcountPtr;
    T* arrayPtr;
};

export enum nofill {};

export typedef glm::tvec3<unsigned char> bytevec3;

export template<class T>
struct Array2D
{
    T* data;
    typedef T value_type;
    int area;
    int w, h;
    int NumBytes() const {
        return area * sizeof(T);
    }
    ivec2 Size() const { return ivec2(w, h); }
    ArrayDeleter<T> deleter;

    Array2D(int w, int h, nofill) : deleter(Init(w, h)) { }
    Array2D(ivec2 s, nofill) : deleter(Init(s.x, s.y)) { }
    Array2D(int w, int h, T const& defaultValue = T()) : deleter(Init(w, h)) { fill(defaultValue); }
    Array2D(ivec2 s, T const& defaultValue = T()) : deleter(Init(s.x, s.y)) { fill(defaultValue); }
    Array2D() : deleter(Init(0, 0)) { }

    T* begin() { return data; }
    T* end() { return data+w*h; }
    T const* begin() const { return data; }
    T const* end() const { return data+w*h; }

    T& operator()(int x, int y) { return data[offsetOf(x, y)]; }
    T const& operator()(int x, int y) const { return data[offsetOf(x, y)]; }

    T& operator()(ivec2 const& v) { return data[offsetOf(v.x, v.y)]; }
    T const& operator()(ivec2 const& v) const { return data[offsetOf(v.x, v.y)]; }

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
    T* Init(int w, int h) {
        auto data = new T[w * h];
        Init(w, h, data);
        return data;
    }
    void Init(int w, int h, T* data) {
        this->data = data;
        area = w * h;
        this->w = w;
        this->h = h;
    }
};

export template<class F> vec3 apply(vec3 const& v, F f)
{
    return vec3(f(v.x), f(v.y), f(v.z));
}

export template<class F> float apply(float v, F f)
{
    return f(v);
}

export const float pi = 3.14159265f;

export template<class T>
T& zero() {
    static T val = T()*0.0f;
    val = T()*0.0f;
    return val;
}

export template<class T>
Array2D<T> empty_like(Array2D<T> a) {
    return Array2D<T>(a.Size(), nofill());
}

export template<class T>
Array2D<T> ones_like(Array2D<T> a) {
    return Array2D<T>(a.Size(), 1.0f);
}

export template<class T>
Array2D<T> zeros_like(Array2D<T> a) {
    return Array2D<T>(a.Size(), zero<T>());
}

export template<class InputIt, class T>
T accumulate(InputIt begin, InputIt end, T base) {
    T sum = base;
    for (auto it = begin; it != end; it++) {
        sum += *it;
    }
    return sum;
}

export template<class T>
void myRemoveIf(vector<T>& vec, function<bool(T const&)> const& pred) {
    vec.erase(std::remove_if(vec.begin(), vec.end(), pred), vec.end());
}

export void rotate(vec2& p, float angle);
export void trapFP();
export float randFloat();

// ---- Implementation ----

void rotate(vec2& p, float angle)
{
    float c = cos(angle), s = sin(angle);
    p = vec2(p.x * c + p.y * (-s), p.x * s + p.y * c);
}

void trapFP()
{
    int cw = _controlfp_s(NULL, 0, 0);
    cw &=~(EM_OVERFLOW|EM_UNDERFLOW|/*EM_INEXACT|*/EM_ZERODIVIDE|EM_DENORMAL);
    _controlfp_s(NULL, cw, MCW_EM);
}

float randFloat()
{
    return rand() / (float)RAND_MAX;
}
