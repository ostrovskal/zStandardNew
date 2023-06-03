
#pragma once

float* z_vec3_mtx(float* v3, float* m);
float* z_vec4_mtx(float* v4, float* m);
float* z_mtx_vec3(float* m, float* v3);
float* z_mtx_vec4(float* m, float* v4);
float* z_mtx_mtx(float* m1, float* m2);

// остаток от деления для вещественных чисел
inline float z_modf(float x, float y) {
	return (x - y *(int)(x / y));
}

inline long z_loop(long x, long y) {
	long z(y / (x * 2));
	long y1 = z * x;
	y -= ((y - y1) >= x) ? y1 * 2 : 0;
	return x - (y < x ? x - y : y - x);
}

inline int z_log2(int i) {
	int value(0);
	while(i >>= 1) { value++; }
	return value;
}

// линейная интерполяция
inline float z_lerp(float f0, float f1, float t) {
	float s = 1.0f - t;
	return f0 * s + f1 * t;
}

// вернуть дробну часть
inline float z_frac(float f) {
	return f - trunc(f);
}

// остаток от целочисленного деления
inline int z_mod(int y, int z) {
	int x = 0;
	for(int i = 0; i < 32; i++) {
		x = (x << 1) | (y >> 31); y = y << 1;
		if((x) >= z) { x = x - z; y = y + 1; }
	}
	return x;
}

#define Z_EPSILON	(1e-06)
#define Z_EPSILON2	(Z_EPSILON * Z_EPSILON)
#define Z_PI		3.14f
#define Z_HALF_PI	(0.5f * Z_PI)
#define Z_DEG2RAD	(Z_PI / 180.0f)
#define Z_RAD2DEG	(180.0f / Z_PI)

class zColor {
public:
	zColor() noexcept { set(0xffffffff); }
	zColor(u32 argb) { set(argb); }
	zColor(cstr str);
	zColor(float _r, float _g, float _b, float _a) { make(_r, _g, _b, _a); }
	operator const float*() const { return vec; }
    float operator [](int index) const { return vec[index]; }
	void make(float _r, float _g, float _b, float _a) { vec[0] = _r; vec[1] = _g; vec[2] = _b; vec[3] = _a; }
	void set(u32 argb);
	u32 toARGB() const;
	u32 toABGR() const;
	const zColor& from(cstr s);
	u8* state(u8* ptr, bool save);
	union {
		struct { float r, g, b, a; };
		float vec[4]{};
	};
	static zColor shadow;
	static zColor red;
	static zColor green;
	static zColor blue;
	static zColor white;
    static zColor gray;
    static zColor black;
};

template<typename T> class zPoint {
public:
	zPoint() noexcept { empty();}
	zPoint(const T& _x, const T& _y) : x(_x), y(_y) { }
	zPoint(const zPoint<T>& p) : x(p.x), y(p.y) { }
    zPoint(const u16* ptr) { set(ptr); }
    zPoint(const u8* ptr) { set(ptr); }
    zPoint(const u32 val) { set(val); }
	void empty() { x = y = 0; }
	bool isEmpty() const { return x == 0 && y == 0; }
	bool isNotEmpty() const { return !isEmpty(); }
	const T& operator [](int index) const { return buf[index]; }
	T& operator [](int index) { return buf[index]; }
	const zPoint<T>& operator = (const zPoint<T>& p) { x = p.x; y = p.y; return *this; }
	const zPoint<T>& operator += (const zPoint<T>& p) { x += p.x; y += p.y; return *this; }
	const zPoint<T>& operator -= (const zPoint<T>& p) { x -= p.x; y -= p.y; return *this; }
	const zPoint<T>& set(const T& _x, const T& _y) { x = _x; y = _y; return *this; }
	const zPoint<T>& set(const u32 _val) { return set((u8*)&_val); }
    template<class R> const zPoint<T>& set(const R* ptr) {
        if(ptr) x = ptr[0], y = ptr[1]; else empty();
        return *this;
    }
	u8* state(u8* ptr, bool save);
    union {
        struct  { T x, y; };
        T buf[2]{};
    };
};

template<typename T> class zSize {
public:
	zSize() noexcept { empty(); }
	zSize(const T& _w, const T& _h) : w(_w), h(_h) { }
	zSize(const zSize<T>& s) : w(s.w), h(s.h) { }
    zSize(const u16* ptr) { set(ptr); }
    zSize(const u8* ptr) { set(ptr); }
    zSize(const u32 val) { set(val); }
	void empty() { w = h = 0; }
	T interval() const { return (h - w) ; }
	T clamp(const T& p) { return (p < w ? w : p > h ? h : p); }
	bool isEmpty() const { return w < 1 || h < 1; }
	bool isNotEmpty() const { return !isEmpty(); }
	const T& operator [](int index) const { return buf[index]; }
	T& operator [](int index) { return buf[index]; }
	zSize<T> operator + (const zSize<T>& s) { return zSize<T>(w + s.w, h + s.h); }
	zSize<T> operator + (const T& s) { return zSize<T>(w + s, h + s); }
	zSize<T> operator - (const zSize<T>& s) { return zSize<T>(w - s.w, h - s.h); }
	zSize<T> operator - (const T& s) { return zSize<T>(w - s, h - s); }
	const zSize<T>& operator = (const zSize<T>& s) { w = s.w; h = s.h; return *this; }
	const zSize<T>& operator += (const zSize<T>& s) { w += s.w; h += s.h; return *this; }
	const zSize<T>& operator -= (const zSize<T>& s) { w -= s.w; h -= s.h; return *this; }
	const zSize<T>& set(const T& _w, const T& _h) { w = _w; h = _h; return *this; }
    const zSize<T>& set(const u32 _val) { return set((u8*)&_val); }
    const zSize<T>& scale(float horz, float vert) { w *= horz; h *= vert; return *this; }
    template<class R> const zSize<T>& set(const R* ptr) {
        if(ptr) w = ptr[0], h = ptr[1]; else empty();
	    return *this;
	}
    u8* state(u8* ptr, bool save);
    union {
        struct  { T w, h; };
        T buf[2]{};
    };
};

template<typename T> class zRect {
public:
	zRect() noexcept { empty(); }
	zRect(const T& _x, const T& _y, const T& _w = 0, const T& _h = 0) : x(_x), y(_y), w(_w), h(_h) { }
    zRect(const zRect<T>& r) : x(r.x), y(r.y), w(r.w), h(r.h) { }
	zRect(const zPoint<T>& p) : x(p.x), y(p.y), w(0), h(0) { }
	zRect(const zSize<T>& s) : x(0), y(0), w(s.w), h(s.h) { }
	zRect(const u16* ptr) { set(ptr); }
	zRect(const u8* ptr) { set(ptr); }
    zRect(const u32 val) { set(val); }
	zPoint<T> xy() const { return zPoint<T>(x, y); }
	zPoint<T> wh() const { return zPoint<T>(w, h); }
	zSize<T> size() const { return zSize<T>(w, h); }
	void empty() { x = y = w = h = 0; }
	bool isEmpty() const { return w < 1 || h < 1; }
	bool isNotEmpty() const { return !isEmpty(); }
	bool contains(T _x, T _y) const { return _x >= x && _y >= y && _x <= (x + w) && _y <= (y + h); }
	bool contains(const zPoint<T>& p) const { return contains(p.x, p.y); }
	bool intersect(const zRect<T>& r) const {
		auto _x(r.x - x); auto t(_x + r.w); auto _w(t - w);
		if(t < 1 || _w >= r.w) return false;
		auto _y(r.y - y); t = (_y + r.h); auto _h(t - h);
		return (t > 0 && _h < r.h);
	}
	operator T*() const { return &x; }
    const T& operator [](int index) const { return buf[index]; }
    T& operator [](int index) { return buf[index]; }
	operator zSize<T>() const { return zSize<T>(w, h); }
	operator zPoint<T>() const { return zPoint<T>(x, y); }
    bool operator == (const zRect<T>& r) const { return x == r.x && y == r.y && w == r.w && h == r.h; }
    bool operator != (const zRect<T>& r) const { return !(operator == (r)); }
	T extent(bool vert) const { return buf[vert] + buf[vert + 2]; }
	zPoint<T> center() const { return zPoint<T>(x + w / 2, y + h / 2); }
	const zRect<T>& operator = (const zRect<T>& r) { x = r.x; y = r.y; w = r.w; h = r.h; return *this; }
	const zRect<T>& operator = (const zPoint<T>& p) { x = p.x; y = p.y; return *this; }
	const zRect<T>& operator = (const zSize<T>& s) { w = s.w; h = s.h; return *this; }
	const zRect<T>& operator * (const T& f) { x *= f; y *= f; w *= f; h *= f; return *this; }
	const zRect<T>& operator += (const zPoint<T>& f) { x += f.x; y += f.y; return *this; }
	const zRect<T>& operator += (const zSize<T>& f) { w += f.w; h += f.h; return *this; }
	const zRect<T>& operator -= (const zPoint<T>& f) { x -= f.x; y -= f.y; return *this; }
	const zRect<T>& operator -= (const zSize<T>& f) { w -= f.w; h -= f.h; return *this; }
	const zRect<T>& operator += (const zRect<T>& f) { x += f.x; y += f.y; return *this; }
	const zRect<T>& operator -= (const zRect<T>& f) { x -= f.x; y -= f.y; return *this; }
	zRect<T> operator + (const zRect<T>& f) { return zRect<T>(x + f.x, y + f.y, w, h); }
	zRect<T> operator - (const zRect<T>& f) { return zRect<T>(x - f.x, y - f.y, w, h); }
	zRect<T> operator + (const zPoint<T>& f) { return zRect<T>(x + f.x, y + f.y, w, h); }
	zRect<T> operator + (const zSize<T>& f) { return zRect<T>(x, y, w + f.w, h + f.h); }
	zRect<T> operator - (const zPoint<T>& f) { return zRect<T>(x - f.x, y - f.y, w, h); }
	zRect<T> operator - (const zSize<T>& f) { return zRect<T>(x, y, w - f.w, h - f.h); }
	const zRect<T>& set(const T& _x, const T& _y, const T& _w, const T& _h) { x = _x; y = _y; w = _w; h = _h; return *this; }
    const zRect<T>& set(const u32 _val) { return set((u8*)&_val); }
	template <typename R> const zRect<T>& set(const R* ptr) {
		if(ptr) x = ptr[0], y = ptr[1], w = ptr[2], h = ptr[3]; else empty();
		return *this;
	}
	const zRect<T>& offset(const T& horz, const T& vert) { x += horz; y += vert; return *this; }
	const zRect<T>& scale(const T& horz, const T& vert) { w *= vert; h *= vert; return *this; }
    zRect<T> padding(const T& horz, const T& vert) { return zRect<T>(x + horz, y + vert, w - horz * 2, h - vert * 2); }
	zRect<T> padding(const zPoint<T> &p) { return zRect<T>(x + p.x, y + p.y, w - p.x * 2, h - p.y * 2); }
	zRect<T> padding(const zRect<T> &r) { return zRect<T>(x + r.x, y + r.y, w - (r.w + r.x), h - (r.h + r.y)); }
	zRect<T> dimens(float sw);
	const zRect<T>& from(cstr s);
	u8* state(u8* ptr, bool save);
    union {
        struct { T x, y, w, h; };
        T buf[4]{};
    };
};

class zMatrix;
class zVec4;
class zQuat;
class zVec3 {
public:
	// конструкторы
	zVec3() : x(0.0f), y(0.0f), z(0.0f) { }
	zVec3(float f) : x(f), y(f), z(f) { }
	zVec3(float X, float Y, float Z) : x(X), y(Y), z(Z) { }
	zVec3(float* f) { x = f[0]; y = f[1]; z = f[2]; }
	zVec3(const zVec3& v) : x(v.x), y(v.y), z(v.z) { }
	// операции
	// унарные
	auto operator + () const { return *this; }
	auto operator - () const { return zVec3(-x, -y, -z); }
	// бинарные
	auto operator + (const zVec3& v) const { return zVec3(x + v.x, y + v.y, z + v.z); }
	auto operator + (float f) const { return zVec3(x + f, y + f, z + f); }
	auto operator - (const zVec3& v) const { return zVec3(x - v.x, y - v.y, z - v.z); }
	auto operator - (float f) const { return zVec3(x - f, y - f, z - f); }
	auto operator * (const zVec3& v) const { return zVec3(x * v.x, y * v.y, z * v.z); }
	auto operator * (const zMatrix& m) const;
	auto operator * (float f) const { return zVec3(x * f, y * f, z * f); }
	auto operator / (const zVec3& v) const { return zVec3(x / v.x, y / v.y, z / v.z); }
	auto operator / (float f) const { f = 1.0f / f; return zVec3(x * f, y * f, z * f); }
	auto operator += (const zVec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	auto operator += (float f) { x += f; y += f; z += f; return *this; }
	auto operator -= (const zVec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	auto operator -= (float f) { x -= f; y -= f; z -= f; return *this; }
	auto operator *= (const zVec3& v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
	auto operator *= (const zMatrix& m);
	auto operator *= (float f) { x *= f; y *= f; z *= f; return *this; }
	auto operator /= (const zVec3& v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
	auto operator /= (float f) { f = 1.0f / f; x *= f; y *= f; z *= f; return *this; }
	// дружественные
	friend zVec3 operator + (float f, const zVec3& v) { return zVec3(f + v.x, f + v.y, f + v.z); }
	friend zVec3 operator - (float f, const zVec3& v) { return zVec3(f - v.x, f - v.y, f - v.z); }
	friend zVec3 operator * (float f, const zVec3& v) { return zVec3(f * v.x, f * v.y, f * v.z); }
	friend zVec3 operator / (float f, const zVec3& v) { return zVec3(f / v.x, f / v.y, f / v.z); }
	friend zVec3 operator * (const zMatrix& m, const zVec3& v);
	friend zVec3 operator * (const zQuat& q, const zVec3& v);
	// логические
	bool operator == (const zVec3& v) const { return ((fabs(x - v.x) + fabs(y - v.y) + fabs(z - v.z)) < Z_EPSILON); }
	bool operator != (const zVec3& v) const { return (!operator == (v)); }
	// присваивание
	auto operator = (const zVec3& v) { x = v.x; y = v.y; z = v.z; return *this; }
	auto operator = (float f) { x = f; y = f; z = f; return *this; }
	float operator[](int idx) const { return vec[idx]; }
	// специальные
	float length() const { return sqrt(x * x + y * y + z * z); }
	float lengthSq() const { return x * x + y * y + z * z; }
	float dot(const zVec3& v) const { return x * v.x + y * v.y + z * v.z; }
	bool isIdentity() const { return (lengthSq() < Z_EPSILON2); }
	auto normalize() { float l(length()); l = (l > Z_EPSILON ? 1.0f / l : 0.0f); x *= l; y *= l; z *= l; return *this; }
	auto floor(const zVec3& v) { if(v.x < x) x = v.x; if(v.y < y) y = v.y; if(v.z < z) z = v.z; return *this; }
	auto ceil(const zVec3& v) { x = ::ceil(v.x); y = ::ceil(v.y); z = ::ceil(v.z); return *this; }
	auto floor() { x = ::floor(x); y = ::floor(y); z = ::floor(z); return *this; }
	auto set(float X, float Y, float Z) { x = X; y = Y; z = Z; return *this; }
	zVec3 cross(const zVec3& v) const { return zVec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
	zVec3 middle(const zVec3& v) const { return zVec3((x + v.x) / 2.0f, (y + v.y) / 2.0f, (z + v.z) / 2.0f); }
	zVec3 reflect(const zVec3& v) const { return zVec3(*this - (2.0f * dot(v) * v)); }
	zVec3 lerp(const zVec3& v, float t) const { float s = 1.0f - t; return zVec3(x * s + t * v.x, y * s + t * v.y, z * s + t * v.z); }
	// приведение типа
	operator float*() const { return (float*)vec; }
	// члены
	union {
		float vec[3]{};
		struct { float x, y, z; };
	};
};

class zVec4 {
public:
	// конструкторы
	zVec4() { identity(); }
	zVec4(float f) : x(f), y(f), z(f), w(f) { }
	zVec4(float X, float Y, float Z, float W) : x(X), y(Y), z(Z), w(W) { }// 0 - vector 1 - point
	zVec4(float* f) { x = f[0]; y = f[1]; z = f[2]; w = f[3]; }
	zVec4(const zVec3& v) : x(v.x), y(v.y), z(v.z), w(0.0f) { }
	zVec4(const zVec4& v) : x(v.x), y(v.y), z(v.z), w(v.w) { }
	// операции
	// унарные
	auto operator + () const { return *this; }
	auto operator - () const { return zVec4(-x, -y, -z, -w); }
	// бинарные
	auto operator + (const zVec4& v) const { return zVec4(x + v.x, y + v.y, z + v.z, w + v.w); }
	auto operator + (float f) const { return zVec4(x + f, y + f, z + f, w + f); }
	auto operator - (const zVec4& v) const { return zVec4(x - v.x, y - v.y, z - v.z, w - v.w); }
	auto operator - (float f) const { return zVec4(x - f, y - f, z - f, w - f); }
	auto operator * (const zVec4& v) const { return zVec4(x * v.x, y * v.y, z * v.z, w * v.w); }
	auto operator * (const zMatrix& m) const;
	auto operator * (float f) const { return zVec4(x * f, y * f, z * f, w * f); }
	auto operator / (const zVec4& v) const { return zVec4(x / v.x, y / v.y, z / v.z, w / v.w); }
	auto operator / (float f) const { f = 1.0f / f; return zVec4(x * f, y * f, z * f, w * f); }
	auto operator += (const zVec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	auto operator += (float f) { x += f; y += f; z += f; w += f; return *this; }
	auto operator -= (const zVec4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	auto operator -= (float f) { x -= f; y -= f; z -= f; w -= f; return *this; }
	auto operator *= (const zVec4& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
	auto operator *= (const zMatrix& m);
	auto operator *= (float f) { x *= f; y *= f; z *= f; w *= f; return *this; }
	auto operator /= (const zVec4& v) { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }
	auto operator /= (float f) { f = 1.0f / f; x *= f; y *= f; z *= f; w *= f; return *this; }
	friend zVec4 operator + (float f, const zVec4& v) { return zVec4(f + v.x, f + v.y, f + v.z, f + v.w); }
	friend zVec4 operator - (float f, const zVec4& v) { return zVec4(f - v.x, f - v.y, f - v.z, f - v.w); }
	friend zVec4 operator * (float f, const zVec4& v) { return zVec4(f * v.x, f * v.y, f * v.z, f * v.w); }
	friend zVec4 operator * (const zMatrix& m, const zVec4& v);
	friend zVec4 operator / (float f, const zVec4& v) { return zVec4(f / v.x, f / v.y, f / v.z, f / v.w); }
	// логические
	bool operator == (const zVec4& v) const { return ((fabs(x - v.x) + fabs(y - v.y) + fabs(z - v.z) + fabs(w - v.w)) < Z_EPSILON); }
	bool operator != (const zVec4& v) const { return !(operator == (v)); }
	// присваивание
	auto operator = (const zVec4& v) { x = v.x; y = v.y; z = v.z; w = v.w; return *this; }
	auto operator = (float f) { x = f; y = f; z = f; w = 0.0f; return *this; }
	float operator[](int idx) const { return vec[idx]; }
	// специальные
	void identity() { x = y = z = w = 0.0f; }
	float length() const { return sqrt(x * x + y * y + z * z + w * w); }
	float lengthSq() const { return x * x + y * y + z * z + w * w; }
	float dot(const zVec4& v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }
	bool isIdentity() const { return(lengthSq() < Z_EPSILON2); }
	auto normalize() { float l(length()); l = (l > Z_EPSILON ? 1.0f / l : 0.0f); x *= l; y *= l; z *= l; w *= l; return *this; }
	auto floor(const zVec4& v) { x = ::floor(v.x); y = ::floor(v.y); z = ::floor(v.z); w = 0.0f; return *this; }
	auto ceil(const zVec4& v) { x = ::ceil(v.x); y = ::ceil(v.y); z = ::ceil(v.z); w = 0.0f; return *this; }
	auto cross(const zVec4& v) const { return zVec4(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x, w); }
	auto middle(const zVec4& v) const { return zVec4((x + v.x) / 2.0f, (y + v.y) / 2.0f, (z + v.z) / 2.0f, w); }
	auto reflect(const zVec4& v) const { return zVec4(*this - (2.0f * dot(v) * v)); }
	auto set(float X, float Y, float Z, float W) { x = X; y = Y; z = Z; w = W; return *this; }
	// приведение типа
	operator float*() const { return (float*)vec; }
	// члены
	union {
		float vec[4]{};
		struct { float x, y, z, w; };
	};
};

class zQuat {
public:
	// конструкторы
	zQuat() { identity(); }
	zQuat(float X, float Y, float Z, float W = 1.0f) : x(X), y(Y), z(Z), w(W) { }
	zQuat(float* f) { x = f[0]; y = f[1]; z = f[2]; w = f[3]; }
	zQuat(const zVec3& v, float f) { rotateAxis(v, f); }
	zQuat(const zVec4& v) : x(v.x), y(v.y), z(v.z), w(v.w) { }
	zQuat(const zQuat& q) : x(q.x), y(q.y), z(q.z), w(q.w) { }
	zQuat(const zMatrix& m) { fromMatrix(m); }
	// операции
	auto operator + () const { return *this; }
	auto operator - () const { return zQuat(-x, -y, -z, -w); }
    float operator [](int index) const { return quat[index]; }
	// бинарные
	auto operator + (const zQuat& q) const { return zQuat(x + q.x, y + q.y, z + q.z, w + q.w); }
	auto operator + (float f) const { return zQuat(x + f, y + f, z + f, w + f); }
	auto operator - (const zQuat& q) const { return zQuat(x - q.x, y - q.y, z - q.z, w - q.w); }
	auto operator - (float f) const { return zQuat(x - f, y - f, z - f, w - f); }
	auto operator * (const zQuat& q) const;
	auto operator * (float f) const { return zQuat(x * f, y * f, z * f, w * f); }
	auto operator / (const zQuat& q) const { return zQuat(x / q.x, y / q.y, z / q.z, w / q.w); }
	auto operator / (float f) const { f = 1.0f / f; return zQuat(x * f, y * f, z * f, w * f); }
	auto operator += (const zQuat& q) { x += q.x; y += q.y; z += q.z; w += q.w; return *this; }
	auto operator += (float f) { x += f; y += f; z += f; w += f; return *this; }
	auto operator -= (const zQuat& q) { x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this; }
	auto operator -= (float f) { x -= f; y -= f; z -= f; w -= f; return *this; }
	auto operator *= (const zQuat& q);
	auto operator *= (float f) { x *= f; y *= f; z *= f; w *= f; return *this; }
	auto operator /= (const zQuat& q) { x /= q.x; y /= q.y; z /= q.z; w /= q.w; return *this; }
	auto operator /= (float f) { f = 1.0f / f; x *= f; y *= f; z *= f; w *= f; return *this; }
	friend zQuat operator + (float f, const zQuat& q) { return zQuat(f + q.x, f + q.y, f + q.z, f + q.w); }
	friend zQuat operator - (float f, const zQuat& q) { return zQuat(f - q.x, f - q.y, f - q.z, f - q.w); }
	friend zQuat operator * (float f, const zQuat& q) { return zQuat(f * q.x, f * q.y, f * q.z, f * q.w); }
	friend zQuat operator / (float f, const zQuat& q) { return zQuat(f / q.x, f / q.y, f / q.z, f / q.w); }
	// логические
	bool operator == (const zQuat& q) const { return ((fabs(x - q.x) + fabs(y - q.y) + fabs(z - q.z) + fabs(w - q.w)) < Z_EPSILON); }
	bool operator != (const zQuat& q) const { return !(operator == (q)); }
	// присваивание
	auto operator = (const zQuat& q) { x = q.x; y = q.y; z = q.z; w = q.w; return *this; }
	// специальные
	bool isIdentity() const { return (fabs(x - y - z) < Z_EPSILON2 && fabs(w - 1) < Z_EPSILON2); }
	float dot(const zQuat& q) const { return w * q.w + x * q.x + y * q.y + z * q.z; }
	float length() const { return sqrtf(x * x + y * y + z * z + w * w); }
	float lengthSq() const { return (x * x + y * y + z * z + w * w); }
	float roll() const { return atan2(2 * (x * y + w * z), w * w + x * x - y * y - z * z); }
	float pitch() const { return atan2(2 * (y * z + w * x), w * w - x * x - y * y + z * z); }
	float yaw() const { return asin(-2 * (x * z - w * y)); }
	auto set(float X, float Y, float Z, float W) { x = X; y = Y; z = Z; w = W; return *this; }
	const zQuat& identity() { x = 0.0f; y = 0.0f; z = 0.0f; w = 1.0f; return *this; }
	auto rotate() const { return zVec3(yaw(), pitch(), roll()); }
	void angleAxis(zVec3& axis, float& angle) const;
	zQuat slerp(const zQuat& q, float t, bool shortestPath) const;
	const zQuat& rotateAxis(const zVec3& v, float theta);
	//Преобразование сферических координат в кватернион
	const zQuat& fromSpherical(float latitude, float longitude, float angle);
	const zQuat& fromMatrix(const zMatrix& mm);
	zVec3 xAxis() const;
	zVec3 yAxis() const;
	zVec3 zAxis() const;
	const zQuat& inverse();
	const zQuat& exp();
	const zQuat& ln();
	const zQuat& angles(float yaw, float pitch, float roll);
	zQuat nlerp(const zQuat& q, float t, bool shortestPath) const;
	zQuat squad(const zQuat& q1, const zQuat& q2, const zQuat& q3, float t, bool shortestPath) const;
	auto normalize() { float factor(1.0f / sqrt(lengthSq())); *this *= factor; return *this; }
	// приведение типа
	operator float * () const { return (float*)&quat; }
	// члены
	union {
		float quat[4]{};
		struct { float x, y, z, w; };
	};
};

class zMatrix {
public:
	// конструкторы
	zMatrix() { identity(); }
	zMatrix(float _f11, float _f12, float _f13, float _f14, float _f21, float _f22, float _f23, float _f24,
			float _f31, float _f32, float _f33, float _f34, float _f41, float _f42, float _f43, float _f44);
	zMatrix(float* mm) { memcpy(*this, mm, sizeof(zMatrix)); }
	zMatrix(const zMatrix& mm) { *this = mm; }
	zMatrix(const zMatrix& pos, const zMatrix& x, const zMatrix& y, const zMatrix& z) { *this = x * y; *this *= z * pos; }
	zMatrix(const zMatrix& scale, const zQuat& rot) { *this = scale * zMatrix(rot); }
	zMatrix(const zVec4& v1, const zVec4& v2, const zVec4& v3, const zVec4& v4) { _v1 = v1; _v2 = v2; _v3 = v3; _v4 = v4; }
	zMatrix(const zQuat& q) { fromQuat(q); }
	// операции
	auto operator + () const { return *this; }
	// бинарные
	zMatrix operator - () const { return zMatrix(-_11, -_12, -_13, -_14, -_21, -_22, -_23, -_24, -_31, -_32, -_33, -_34, -_41, -_42, -_43, -_44); }
	zMatrix operator + (float f) const { return zMatrix(_v1 + f, _v2 + f, _v3 + f, _v4 + f); }
	zMatrix operator - (float f) const { return zMatrix(_v1 - f, _v2 - f, _v3 - f, _v4 - f); }
	zMatrix operator * (float f) const { return zMatrix( _v1 * f, _v2 * f, _v3 * f, _v4 * f); }
	zMatrix operator / (float f) const { return zMatrix( _v1 / f, _v2 / f, _v3 / f, _v4 / f); }
	zMatrix operator + (const zMatrix& m) const { return zMatrix(_v1 - m._v1, _v2 - m._v2, _v3 - m._v3, _v4 - m._v4); }
	zMatrix operator - (const zMatrix& m) const { return zMatrix(_v1 - m._v1, _v2 - m._v2, _v3 - m._v3, _v4 - m._v4); }
	zMatrix operator / (const zMatrix& m) const { return zMatrix(_v1 / m._v1, _v2 / m._v2, _v3 / m._v3, _v4 / m._v4); }
	zMatrix operator * (const zMatrix& m) const { return zMatrix(z_mtx_mtx(*this, m)); }
	const zMatrix& operator += (float f) { _v1 += f; _v2 += f; _v3 += f; _v4 += f; return *this; }
	const zMatrix& operator -= (float f) { _v1 -= f; _v2 -= f; _v3 -= f; _v4 -= f; return *this; }
	const zMatrix& operator *= (float f) { _v1 *= f; _v2 *= f; _v3 *= f; _v4 *= f; return *this; }
	const zMatrix& operator /= (float f) { _v1 /= f; _v2 /= f; _v3 /= f; _v4 /= f; return *this; }
	const zMatrix& operator += (const zMatrix& m) { _v1 += m._v1; _v2 += m._v2; _v3 += m._v3; _v4 += m._v4; return *this; }
	const zMatrix& operator -= (const zMatrix& m) { _v1 -= m._v1; _v2 -= m._v2; _v3 -= m._v3; _v4 -= m._v4; return *this; }
	const zMatrix& operator *= (const zMatrix& m) { *this = z_mtx_mtx(*this, m); return *this; }
	// дружественные
	friend zMatrix operator + (float f, const zMatrix& m) { return zMatrix( f + m._v1, f + m._v2, f + m._v3, f + m._v4); }
	friend zMatrix operator - (float f, const zMatrix& m) { return zMatrix( f - m._v1, f - m._v2, f - m._v3, f - m._v4); }
	friend zMatrix operator * (float f, const zMatrix& m) { return zMatrix( f * m._v1, f * m._v2, f * m._v3, f * m._v4); }
	friend zMatrix operator / (float f, const zMatrix& m) { return zMatrix( f / m._v1, f / m._v2, f / m._v3, f / m._v4); }
	// присваивание
	const zMatrix& operator = (const zMatrix& m) { memcpy(*this, &m, sizeof(zMatrix)); return *this; }
	// извлечения
	float operator [] (int idx) const { return mtx[idx]; }
	// сравнения
	bool operator == (const zMatrix& m) { return (memcmp(*this, &m, sizeof(zMatrix)) == 0); }
	bool operator != (const zMatrix& m) { return !(operator == (m)); }
	// проверка на единичную матрицу
	bool isIdent() const { return memcmp(&_identity, *this, 16) == 0; }
	// специальные
	float MINOR(const zMatrix& m, int r0, int r1, int r2, int c0, int c1, int c2) const;
	// определитель
	float determinant() const;
	zMatrix adjoint() const;
	// инвертировать
	auto inverse();
	// матрица вида
	const zMatrix& view(const zVec3& pos, const zVec3& at, const zVec3& up);
	// из кватерниона
	const zMatrix& fromQuat(const zQuat& q);
	// мировая матрица
	const zMatrix& world(const zVec3& pos, const zVec3& scale, const zQuat& orientation);
	// ортогональная матрица
	const zMatrix& ortho(int left, int right, int top, int bottom);
	const zMatrix& ortho2D(int left, int top, int right, int bottom);
	// перспективная матрица
	const zMatrix& perspective(float w, float h, float zn, float zf);
	// перспективная матрица из угла камеры
	const zMatrix& perspectiveFov(float fovy, float aspect, float zn, float zf);
	// перевернуть
	const zMatrix& transpose();
	// единичная
	const zMatrix& identity() { memcpy(*this, &_identity, sizeof(zMatrix)); return *this; }
	// матрица позиции
	auto translate(float x, float y, float z) { identity(); _41 = x; _42 = y; _43 = z; return *this; }
	auto translate(const zVec3& v) { return translate(v.x, v.y, v.z); }
	// матрица масштабирования
	const zMatrix& scale(float x, float y, float z) { identity(); _11 = x; _22 = y; _33 = z; return *this; }
	auto scale(const zVec3& v) { return scale(v.x, v.y, v.z); }
	// установить позицию
	auto translate2(float x, float y, float z) { _41 = x; _42 = y; _43 = z; return *this; }
	auto translate2(const zVec3& v) { _41 = v.x; _42 = v.y; _43 = v.z; return *this; }
	// установить масштаб
	auto scale2(float x, float y, float z) { _11 = x; _22 = y; _33 = z; return *this; }
	auto scale2(const zVec3& v) { _11 = v.x; _22 = v.y; _33 = v.z; return *this; }
	// создать матрицу поворота из углов Эйлера
	const zMatrix& rotateEuler(float yaw, float pitch, float roll);
	// повернуть из произвольной оси и угла
	const zMatrix& rotateAngleAxis(const zVec3& v, float angle);
	const zMatrix& postTranslate(float x, float y, float z);
    // поворот по осям
    const zMatrix& rotate(const zVec3& v) { return rotate(v.x, v.y, v.z); }
    const zMatrix& rotate(float x, float y, float z);
	// вернуть смещение
	auto getTranslate() const { return zVec3(_41, _42, _43); }
	// вернуть масштаб
	auto getScale() const { return zVec3(_11, _22, _33); }
	// вернуть углы Эйлера
	auto rotateEuler2() const { zQuat q(*this); return q.rotate(); }
	// преобразовать из матрицы 3x3
	const zMatrix& from3x3(float* f);
	// приведение типа
	operator float * () const { return (float*)mtx; }
	// единичная матрица
	static zMatrix _identity;
	union {
		float mtx[16]{};
		struct { zVec4 _v1, _v2, _v3, _v4; };
		struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
	};
};

inline auto zVec3::operator * (const zMatrix& m) const { return zVec3(z_vec3_mtx(*this, m)); }
inline zVec3 operator * (const zMatrix& m, const zVec3& v) { return zVec3(z_mtx_vec3(m, v)); }
inline auto zVec4::operator * (const zMatrix& m) const { return zVec4(z_vec4_mtx(*this, m)); }
inline zVec4 operator * (const zMatrix& m, const zVec4& v) { return zVec4(z_mtx_vec4(m, v)); }
inline auto zVec3::operator *= (const zMatrix& m) { *this = z_vec3_mtx(*this, m); return *this; }
inline auto zVec4::operator *= (const zMatrix& m) { *this = z_vec4_mtx(*this, m); return *this; }


using rti = zRect<int>;
using rtf = zRect<float>;
using szi = zSize<int>;
using szf = zSize<float>;
using pti = zPoint<int>;
using ptf = zPoint<float>;
using crti = const zRect<int>;
using crtf = const zRect<float>;
using cszi = const zSize<int>;
using cszf = const zSize<float>;
using cpti = const zPoint<int>;
using cptf = const zPoint<float>;

class zVertex2D {
public:
	zVertex2D() {}
	float x{0.0f}, y{0.0f};
	float u{0.0f}, v{0.0f};
};
