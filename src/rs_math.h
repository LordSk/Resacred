#pragma once
#include <math.h> // c math
#include "rs_base.h"
#ifdef RS_MATH_SIMD_IMPL
	#include <intrin.h>
#endif

typedef float f32;

#ifndef RS_PI
	#define RS_PI (3.14159265359)
	#define RS_2PI (6.28318530718)
	#define RS_HALFPI (1.57079632679)
#endif

inline f32 rs_radians(f32 deg)
{
	return deg * RS_PI / 180.f;
}

inline f32 rs_modf(f32 val, f32 m)
{
	if(val > m) return val - (i32)(val / m) * m;
	return val + (i32)(-val / m) * m;
}

// TODO: inverse a1 and a2?
inline f32 rs_lerpAngle(f32 a1, f32 a2, f32 alpha)
{
	f32 angle1 = rs_modf(a1, RS_2PI);
	f32 angle2 = rs_modf(a2, RS_2PI);
	f32 angleDiff = angle1 - angle2;
	angleDiff = angleDiff < 0 ? -angleDiff : angleDiff; // abs

	if(angleDiff > RS_PI) {
		if(angle2 > angle1) {
			angle1 += RS_2PI;
		}
		else{
			angle2 += RS_2PI;
		}
	}

	return angle1 * alpha + angle2 * (1.f - alpha);
}

#ifndef rs_min
	#define rs_min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef rs_max
	#define rs_max(a, b) ((a) > (b) ? (a) : (b))
#endif

//
// Vector 2 type
//
union alignas(16) vec2f
{
	struct { f32 x, y, unused[2]; };
	f32 data[4];
#ifdef RS_MATH_SIMD_IMPL
	__m128 wide;
#endif

    vec2f() = default;

    constexpr vec2f(f32 x_, f32 y_)
	: data{x_, y_, 0, 0} {
	}

    constexpr explicit vec2f(f32 one)
	: data{one, one, 0, 0} {
	}
};

inline vec2f vec2fMinus(const vec2f& v)
{
#ifdef RS_MATH_SIMD_IMPL
    vec2f r;
	r.wide = _mm_sub_ps(_mm_set1_ps(0.f), v.wide);
	return r;
#else
    return vec2f{
		-v.x,
		-v.y
	};
#endif
}

inline vec2f vec2fAdd(const vec2f& v1, const vec2f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec2f r;
	r.wide = _mm_add_ps(v1.wide, v2.wide);
	return r;
#else
    return vec2f{
		v1.x + v2.x,
		v1.y + v2.y
	};
#endif
}

inline vec2f* vec2fAdd(vec2f* v, const vec2f& other)
{
	v->x += other.x;
	v->y += other.y;
	return v;
}

inline vec2f vec2fSub(const vec2f& v1, const vec2f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec2f r;
	r.wide = _mm_sub_ps(v1.wide, v2.wide);
	return r;
#else
    return vec2f{
		v1.x - v2.x,
		v1.y - v2.y
	};
#endif
}

inline vec2f* vec2fSub(vec2f* v, const vec2f& other)
{
	v->x -= other.x;
	v->y -= other.y;
	return v;
}

inline vec2f vec2fMul(const vec2f& v1, const vec2f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec2f r;
	r.wide = _mm_mul_ps(v1.wide, v2.wide);
	return r;
#else
    return vec2f{
		v1.x * v2.x,
		v1.y * v2.y
	};
#endif
}

inline vec2f* vec2fMul(vec2f* v, const vec2f& other)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_mul_ps(v->wide, other.wide);
#else
	v->x *= other.x;
	v->y *= other.y;
#endif
	return v;
}

inline vec2f vec2fDiv(const vec2f& v1, const vec2f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec2f r;
	r.wide = _mm_div_ps(v1.wide, v2.wide);
	return r;
#else
    return vec2f{
		v1.x / v2.x,
		v1.y / v2.y
	};
#endif
}

inline vec2f* vec2fDiv(vec2f* v, const vec2f& other)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_div_ps(v->wide, other.wide);
#else
	v->x /= other.x;
	v->y /= other.y;
#endif
	return v;
}

// scalar
inline vec2f vec2fAdd(const vec2f& v, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
    vec2f r;
	r.wide = _mm_add_ps(v.wide, _mm_set1_ps(scalar));
	return r;
#else
    return vec2f{
		v.x + scalar,
		v.y + scalar
	};
#endif
}

inline vec2f* vec2fAdd(vec2f* v, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_add_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x += scalar;
	v->y += scalar;
#endif
	return v;
}


inline vec2f vec2fSub(const vec2f& v1, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
    vec2f r;
	r.wide = _mm_sub_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
    return vec2f{
		v1.x - scalar,
		v1.y - scalar
	};
#endif
}

inline vec2f* vec2fSub(vec2f* v, f32 scalar)
{
	v->x -= scalar;
	v->y -= scalar;
	return v;
}

inline vec2f vec2fMul(const vec2f& v1, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
    vec2f r;
	r.wide = _mm_mul_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
    return vec2f{
		v1.x * scalar,
		v1.y * scalar
	};
#endif
}

inline vec2f* vec2fMul(vec2f* v, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_mul_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x *= scalar;
	v->y *= scalar;
#endif
	return v;
}

inline vec2f vec2fDiv(const vec2f& v1, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
    vec2f r;
	r.wide = _mm_div_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
    return vec2f{
		v1.x / scalar,
		v1.y / scalar
	};
#endif
}

inline vec2f* vec2fDiv(vec2f* v, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_div_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x /= scalar;
	v->y /= scalar;
#endif
	return v;
}

inline f32 rs_dot(const vec2f& v1, const vec2f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
	__m128 r = _mm_mul_ps(v1.wide, v2.wide);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	return ((f32*)&r)[0];
#else
	return v1.x*v2.x + v1.y*v2.y;
#endif
}

inline f32 rs_lengthSq(const vec2f& v)
{
	return rs_dot(v , v);
}

inline f32 rs_length(const vec2f& v)
{
#ifdef RS_MATH_SIMD_IMPL
	__m128 r = _mm_mul_ps(v.wide, v.wide);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	r = _mm_sqrt_ps(r);
	return ((f32*)&r)[0];
#else
	return sqrtf(rs_lengthSq(v));
#endif
}

inline f32 rs_distance(const vec2f& v1, const vec2f& v2)
{
    return rs_length(vec2fSub(v1, v2));
}

inline vec2f rs_normalize(vec2f v)
{
#ifdef RS_MATH_SIMD_IMPL
	__m128 len = _mm_mul_ps(v.wide, v.wide);
	len = _mm_hadd_ps(len, len);
	len = _mm_hadd_ps(len, len);
	len = _mm_sqrt_ps(len);
	v.wide = _mm_div_ps(v.wide, len);
	return v;
#else
	f32 scale = 1.0f / rs_length(v);
    return vec2fMul(v, scale);
#endif
}

#ifdef RS_MATH_OPERATORS
inline vec2f operator-(const vec2f& v) {
    return vec2fMinus(v);
}

inline vec2f operator+(const vec2f& v1, const vec2f& v2) {
    return vec2fAdd(v1, v2);
}

inline vec2f& operator+=(vec2f& v, const vec2f& other) {
    return *vec2fAdd(&v, other);
}

inline vec2f operator-(const vec2f& v1, const vec2f& v2) {
    return vec2fSub(v1, v2);
}

inline vec2f& operator-=(vec2f& v, const vec2f& other) {
    return *vec2fSub(&v, other);
}

inline vec2f operator*(const vec2f& v1, const vec2f& v2) {
    return vec2fMul(v1, v2);
}

inline vec2f& operator*=(vec2f& v, const vec2f& other) {
    return *vec2fMul(&v, other);
}

inline vec2f operator/(const vec2f& v1, const vec2f& v2) {
    return vec2fDiv(v1, v2);
}

inline vec2f& operator/=(vec2f& v, const vec2f& other) {
    return *vec2fDiv(&v, other);
}

// scalar
inline vec2f operator+(const vec2f& v, f32 scalar) {
    return vec2fAdd(v, scalar);
}

inline vec2f& operator+=(vec2f& v, f32 scalar) {
    return *vec2fAdd(&v, scalar);
}

inline vec2f operator-(const vec2f& v1, f32 scalar) {
    return vec2fSub(v1, scalar);
}

inline vec2f& operator-=(vec2f& v, f32 scalar) {
    return *vec2fSub(&v, scalar);
}

inline vec2f operator*(const vec2f& v1, f32 scalar) {
    return vec2fMul(v1, scalar);
}

inline vec2f& operator*=(vec2f& v, f32 scalar) {
    return *vec2fMul(&v, scalar);
}

inline vec2f operator/(const vec2f& v1, f32 scalar) {
    return vec2fDiv(v1, scalar);
}

inline vec2f& operator/=(vec2f& v, f32 scalar) {
    return *vec2fDiv(&v, scalar);
}
#endif


//
// Vector 3
//
union alignas(16) vec3f
{
	struct { f32 x, y, z, unused; };
	f32 data[4];
#ifdef RS_MATH_SIMD_IMPL
	__m128 wide;
#endif

    vec3f() = default;

    constexpr vec3f(f32 x_, f32 y_, f32 z_)
	: data{x_, y_, z_, 0} {
	}

    constexpr explicit vec3f(f32 one)
	: data{one, one, one, 0} {
	}

    constexpr vec3f(const vec2f& v2, f32 z_)
	: data{v2.x, v2.y, z_, 0} {
	}
};

inline vec3f vec3fMinus(const vec3f& v)
{
#ifdef RS_MATH_SIMD_IMPL
    vec3f r;
	r.wide = _mm_sub_ps(_mm_set1_ps(0.f), v.wide);
	return r;
#else
    return vec3f{
		-v.x,
		-v.y,
		-v.z
	};
#endif
}

inline vec3f vec3fAdd(const vec3f& v1, const vec3f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec3f r;
	r.wide = _mm_add_ps(v1.wide, v2.wide);
	return r;
#else
    return vec3f{
		v1.x + v2.x,
		v1.y + v2.y,
		v1.z + v2.z
	};
#endif
}

inline vec3f* vec3fAdd(vec3f* v, const vec3f& other)
{
	v->x += other.x;
	v->y += other.y;
	v->z += other.z;
	return v;
}

inline vec3f vec3fSub(const vec3f& v1, const vec3f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec3f r;
	r.wide = _mm_sub_ps(v1.wide, v2.wide);
	return r;
#else
    return vec3f{
		v1.x - v2.x,
		v1.y - v2.y,
		v1.z - v2.z
	};
#endif
}

inline vec3f* vec3fSub(vec3f* v, const vec3f& other)
{
	v->x -= other.x;
	v->y -= other.y;
	v->z -= other.z;
	return v;
}

inline vec3f vec3fMul(const vec3f& v1, const vec3f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec3f r;
	r.wide = _mm_mul_ps(v1.wide, v2.wide);
	return r;
#else
    return vec3f{
		v1.x * v2.x,
		v1.y * v2.y,
		v1.z * v2.z
	};
#endif
}

inline vec3f* vec3fMul(vec3f* v, const vec3f& other)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_mul_ps(v->wide, other.wide);
#else
	v->x *= other.x;
	v->y *= other.y;
	v->z *= other.z;
#endif
	return v;
}

inline vec3f vec3fDiv(const vec3f& v1, const vec3f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec3f r;
	r.wide = _mm_div_ps(v1.wide, v2.wide);
	return r;
#else
    return vec3f{
		v1.x / v2.x,
		v1.y / v2.y,
		v1.z / v2.z
	};
#endif
}

inline vec3f* vec3fDiv(vec3f* v, const vec3f& other)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_div_ps(v->wide, other.wide);
#else
	v->x /= other.x;
	v->y /= other.y;
	v->z /= other.z;
#endif
	return v;
}

// scalar
inline vec3f vec3fAdd(const vec3f& v, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
    vec3f r;
	r.wide = _mm_add_ps(v.wide, _mm_set1_ps(scalar));
	return r;
#else
    return vec3f{
		v.x + scalar,
		v.y + scalar,
		v.z + scalar
	};
#endif
}

inline vec3f* vec3fAdd(vec3f* v, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_add_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x += scalar;
	v->y += scalar;
	v->z += scalar;
#endif
	return v;
}


inline vec3f vec3fSub(const vec3f& v1, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
    vec3f r;
	r.wide = _mm_sub_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
    return vec3f{
		v1.x - scalar,
		v1.y - scalar,
		v1.z - scalar
	};
#endif
}

inline vec3f* vec3fSub(vec3f* v, f32 scalar)
{
	v->x -= scalar;
	v->y -= scalar;
	v->z -= scalar;
	return v;
}

inline vec3f vec3fMul(const vec3f& v1, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
    vec3f r;
	r.wide = _mm_mul_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
    return vec3f{
		v1.x * scalar,
		v1.y * scalar,
		v1.z * scalar
	};
#endif
}

inline vec3f* vec3fMul(vec3f* v, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_mul_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x *= scalar;
	v->y *= scalar;
	v->z *= scalar;
#endif
	return v;
}

inline vec3f vec3fDiv(const vec3f& v1, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
    vec3f r;
	r.wide = _mm_div_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
    return vec3f{
		v1.x / scalar,
		v1.y / scalar,
		v1.z / scalar
	};
#endif
}

inline vec3f* vec3fDiv(vec3f* v, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_div_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x /= scalar;
	v->y /= scalar;
	v->z /= scalar;
#endif
	return v;
}

inline vec3f rs_cross(const vec3f& v1, const vec3f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
	__m128 v1s0 = _mm_shuffle_ps(v1.wide, v1.wide, _MM_SHUFFLE(0, 0, 2, 1));
	__m128 v2s0 = _mm_shuffle_ps(v2.wide, v2.wide, _MM_SHUFFLE(0, 1, 0, 2));
	__m128 v1s1 = _mm_shuffle_ps(v1.wide, v1.wide, _MM_SHUFFLE(0, 1, 0, 2));
	__m128 v2s1 = _mm_shuffle_ps(v2.wide, v2.wide, _MM_SHUFFLE(0, 0, 2, 1));
    vec3f r;
	r.wide = _mm_sub_ps(_mm_mul_ps(v1s0, v2s0), _mm_mul_ps(v1s1, v2s1));
	return r;
#else
    return vec3f{
		(v1.y * v2.z) - (v1.z * v2.y),
		(v1.z * v2.x) - (v1.x * v2.z),
		(v1.x * v2.y) - (v1.y * v2.x)
	};
#endif
}

inline f32 rs_dot(const vec3f& v1, const vec3f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
	__m128 r = _mm_mul_ps(v1.wide, v2.wide);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	return ((f32*)&r)[0];
#else
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
#endif
}

inline f32 rs_lengthSq(const vec3f& v)
{
	return rs_dot(v , v);
}

inline f32 rs_length(const vec3f& v)
{
#ifdef RS_MATH_SIMD_IMPL
	__m128 r = _mm_mul_ps(v.wide, v.wide);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	r = _mm_sqrt_ps(r);
	return ((f32*)&r)[0];
#else
	return sqrtf(rs_lengthSq(v));
#endif
}

inline f32 rs_distance(const vec3f& v1, const vec3f& v2)
{
    return rs_length(vec3fSub(v1, v2));
}

inline vec3f rs_normalize(vec3f v)
{
#ifdef RS_MATH_SIMD_IMPL
	__m128 len = _mm_mul_ps(v.wide, v.wide);
	len = _mm_hadd_ps(len, len);
	len = _mm_hadd_ps(len, len);
	len = _mm_sqrt_ps(len);
	v.wide = _mm_div_ps(v.wide, len);
	return v;
#else
	f32 scale = 1.0f / rs_length(v);
    return vec3fMul(v, scale);
#endif
}

inline vec3f vec3fMin(const vec3f& v1, const vec3f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec3f vmin;
	vmin.wide = _mm_min_ps(v1.wide, v2.wide);
	return vmin;
#else
    vec3f vmin;
	vmin.x = rs_min(v1.x, v2.x);
	vmin.y = rs_min(v1.y, v2.y);
	vmin.z = rs_min(v1.z, v2.z);
	return vmin;
#endif
}

inline vec3f vec3fMax(const vec3f& v1, const vec3f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec3f vmax;
	vmax.wide = _mm_max_ps(v1.wide, v2.wide);
	return vmax;
#else
    vec3f vmax;
    vmax.x = rs_max(v1.x, v2.x);
    vmax.y = rs_max(v1.y, v2.y);
    vmax.z = rs_max(v1.z, v2.z);
	return vmax;
#endif
}

#ifdef RS_MATH_OPERATORS
inline vec3f operator-(const vec3f& v) {
    return vec3fMinus(v);
}

inline vec3f operator+(const vec3f& v1, const vec3f& v2) {
    return vec3fAdd(v1, v2);
}

inline vec3f& operator+=(vec3f& v, const vec3f& other) {
    return *vec3fAdd(&v, other);
}

inline vec3f operator-(const vec3f& v1, const vec3f& v2) {
    return vec3fSub(v1, v2);
}

inline vec3f& operator-=(vec3f& v, const vec3f& other) {
    return *vec3fSub(&v, other);
}

inline vec3f operator*(const vec3f& v1, const vec3f& v2) {
    return vec3fMul(v1, v2);
}

inline vec3f& operator*=(vec3f& v, const vec3f& other) {
    return *vec3fMul(&v, other);
}

inline vec3f operator/(const vec3f& v1, const vec3f& v2) {
    return vec3fDiv(v1, v2);
}

inline vec3f& operator/=(vec3f& v, const vec3f& other) {
    return *vec3fDiv(&v, other);
}

// scalar
inline vec3f operator+(const vec3f& v, f32 scalar) {
    return vec3fAdd(v, scalar);
}

inline vec3f& operator+=(vec3f& v, f32 scalar) {
    return *vec3fAdd(&v, scalar);
}

inline vec3f operator-(const vec3f& v1, f32 scalar) {
    return vec3fSub(v1, scalar);
}

inline vec3f& operator-=(vec3f& v, f32 scalar) {
    return *vec3fSub(&v, scalar);
}

inline vec3f operator*(const vec3f& v1, f32 scalar) {
    return vec3fMul(v1, scalar);
}

inline vec3f& operator*=(vec3f& v, f32 scalar) {
    return *vec3fMul(&v, scalar);
}

inline vec3f operator/(const vec3f& v1, f32 scalar) {
    return vec3fDiv(v1, scalar);
}

inline vec3f& operator/=(vec3f& v, f32 scalar) {
    return *vec3fDiv(&v, scalar);
}
#endif



//
// Vector 4
//
union alignas(16) vec4f
{
	struct { f32 x, y, z, w; };
	struct { f32 r, g, b, a; };
	f32 data[4];
#ifdef RS_MATH_SIMD_IMPL
	__m128 wide;
#endif

    vec4f() = default;

    constexpr vec4f(f32 x_, f32 y_, f32 z_, f32 w_)
	: data{x_, y_, z_, w_} {
	}

    constexpr explicit vec4f(f32 one)
	: data{one, one, one, one} {
	}

    constexpr explicit vec4f(const vec3f& v3, f32 w_)
	: data{v3.x, v3.y, v3.z, w_} {
	}
};

inline vec4f vec4fMinus(const vec4f& v)
{
#ifdef RS_MATH_SIMD_IMPL
    vec4f r;
	r.wide = _mm_sub_ps(_mm_set1_ps(0.f), v.wide);
	return r;
#else
    return vec4f{
		-v.x,
		-v.y,
		-v.z,
		-v.w
	};
#endif
}

inline vec4f vec4fAdd(const vec4f& v1, const vec4f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec4f r;
	r.wide = _mm_add_ps(v1.wide, v2.wide);
	return r;
#else
    return vec4f{
		v1.x + v2.x,
		v1.y + v2.y,
		v1.z + v2.z,
		v1.w + v2.w
	};
#endif
}

inline vec4f* vec4fAdd(vec4f* v, const vec4f& other)
{
	v->x += other.x;
	v->y += other.y;
	v->z += other.z;
	v->w += other.w;
	return v;
}

inline vec4f vec4fSub(const vec4f& v1, const vec4f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec4f r;
	r.wide = _mm_sub_ps(v1.wide, v2.wide);
	return r;
#else
    return vec4f{
		v1.x - v2.x,
		v1.y - v2.y,
		v1.z - v2.z,
		v1.w - v2.w
	};
#endif
}

inline vec4f* vec4fSub(vec4f* v, const vec4f& other)
{
	v->x -= other.x;
	v->y -= other.y;
	v->z -= other.z;
	v->w -= other.w;
	return v;
}

inline vec4f vec4fMul(const vec4f& v1, const vec4f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec4f r;
	r.wide = _mm_mul_ps(v1.wide, v2.wide);
	return r;
#else
    return vec4f{
		v1.x * v2.x,
		v1.y * v2.y,
		v1.z * v2.z,
		v1.w * v2.w
	};
#endif
}

inline vec4f* vec4fMul(vec4f* v, const vec4f& other)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_mul_ps(v->wide, other.wide);
#else
	v->x *= other.x;
	v->y *= other.y;
	v->z *= other.z;
	v->w *= other.w;
#endif
	return v;
}

inline vec4f vec4fDiv(const vec4f& v1, const vec4f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
    vec4f r;
	r.wide = _mm_div_ps(v1.wide, v2.wide);
	return r;
#else
    return vec4f{
		v1.x / v2.x,
		v1.y / v2.y,
		v1.z / v2.z,
		v1.w / v2.w
	};
#endif
}

inline vec4f* vec4fDiv(vec4f* v, const vec4f& other)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_div_ps(v->wide, other.wide);
#else
	v->x /= other.x;
	v->y /= other.y;
	v->z /= other.z;
	v->w /= other.w;
#endif
	return v;
}

// scalar
inline vec4f vec4fAdd(const vec4f& v, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
    vec4f r;
	r.wide = _mm_add_ps(v.wide, _mm_set1_ps(scalar));
	return r;
#else
    return vec4f{
		v.x + scalar,
		v.y + scalar,
		v.z + scalar,
		v.w + scalar
	};
#endif
}

inline vec4f* vec4fAdd(vec4f* v, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_add_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x += scalar;
	v->y += scalar;
	v->z += scalar;
	v->w += scalar;
#endif
	return v;
}


inline vec4f vec4fSub(const vec4f& v1, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
    vec4f r;
	r.wide = _mm_sub_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
    return vec4f{
		v1.x - scalar,
		v1.y - scalar,
		v1.z - scalar,
		v1.w - scalar
	};
#endif
}

inline vec4f* vec4fSub(vec4f* v, f32 scalar)
{
	v->x -= scalar;
	v->y -= scalar;
	v->z -= scalar;
	v->w -= scalar;
	return v;
}

inline vec4f vec4fMul(const vec4f& v1, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
    vec4f r;
	r.wide = _mm_mul_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
    return vec4f{
		v1.x * scalar,
		v1.y * scalar,
		v1.z * scalar,
		v1.w * scalar
	};
#endif
}

inline vec4f* vec4fMul(vec4f* v, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_mul_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x *= scalar;
	v->y *= scalar;
	v->z *= scalar;
	v->w *= scalar;
#endif
	return v;
}

inline vec4f vec4fDiv(const vec4f& v1, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
    vec4f r;
	r.wide = _mm_div_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
    return vec4f{
		v1.x / scalar,
		v1.y / scalar,
		v1.z / scalar,
		v1.w / scalar
	};
#endif
}

inline vec4f* vec4fDiv(vec4f* v, f32 scalar)
{
#ifdef RS_MATH_SIMD_IMPL
	v->wide = _mm_div_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x /= scalar;
	v->y /= scalar;
	v->z /= scalar;
	v->w /= scalar;
#endif
	return v;
}

inline f32 rs_dot(const vec4f& v1, const vec4f& v2)
{
#ifdef RS_MATH_SIMD_IMPL
	__m128 r = _mm_mul_ps(v1.wide, v2.wide);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	return ((f32*)&r)[0];
#else
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w;
#endif
}

inline f32 rs_lengthSq(const vec4f& v)
{
	return rs_dot(v , v);
}

inline f32 rs_length(const vec4f& v)
{
#ifdef RS_MATH_SIMD_IMPL
	__m128 r = _mm_mul_ps(v.wide, v.wide);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	r = _mm_sqrt_ps(r);
	return ((f32*)&r)[0];
#else
	return sqrtf(rs_lengthSq(v));
#endif
}

inline vec4f rs_normalize(vec4f v)
{
#ifdef RS_MATH_SIMD_IMPL
	__m128 len = _mm_mul_ps(v.wide, v.wide);
	len = _mm_hadd_ps(len, len);
	len = _mm_hadd_ps(len, len);
	len = _mm_sqrt_ps(len);
	v.wide = _mm_div_ps(v.wide, len);
	return v;
#else
	f32 scale = 1.0f / rs_length(v);
    return vec4fMul(v, scale);
#endif
}

#ifdef RS_MATH_OPERATORS
inline vec4f operator-(const vec4f& v) {
    return vec4fMinus(v);
}

inline vec4f operator+(const vec4f& v1, const vec4f& v2) {
    return vec4fAdd(v1, v2);
}

inline vec4f& operator+=(vec4f& v, const vec4f& other) {
    return *vec4fAdd(&v, other);
}

inline vec4f operator-(const vec4f& v1, const vec4f& v2) {
    return vec4fSub(v1, v2);
}

inline vec4f& operator-=(vec4f& v, const vec4f& other) {
    return *vec4fSub(&v, other);
}

inline vec4f operator*(const vec4f& v1, const vec4f& v2) {
    return vec4fMul(v1, v2);
}

inline vec4f& operator*=(vec4f& v, const vec4f& other) {
    return *vec4fMul(&v, other);
}

inline vec4f operator/(const vec4f& v1, const vec4f& v2) {
    return vec4fDiv(v1, v2);
}

inline vec4f& operator/=(vec4f& v, const vec4f& other) {
    return *vec4fDiv(&v, other);
}

// scalar
inline vec4f operator+(const vec4f& v, f32 scalar) {
    return vec4fAdd(v, scalar);
}

inline vec4f& operator+=(vec4f& v, f32 scalar) {
    return *vec4fAdd(&v, scalar);
}

inline vec4f operator-(const vec4f& v1, f32 scalar) {
    return vec4fSub(v1, scalar);
}

inline vec4f& operator-=(vec4f& v, f32 scalar) {
    return *vec4fSub(&v, scalar);
}

inline vec4f operator*(const vec4f& v1, f32 scalar) {
    return vec4fMul(v1, scalar);
}

inline vec4f& operator*=(vec4f& v, f32 scalar) {
    return *vec4fMul(&v, scalar);
}

inline vec4f operator/(const vec4f& v1, f32 scalar) {
    return vec4fDiv(v1, scalar);
}

inline vec4f& operator/=(vec4f& v, f32 scalar) {
    return *vec4fDiv(&v, scalar);
}
#endif

// matrix

//
// Matrix 4x4
//
union alignas(16) mat4
{
	f32 data[16];
#ifdef RS_MATH_SIMD_IMPL
	__m128 wide[4];
#endif
};

inline mat4 mat4Identity()
{
    return mat4{
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

inline mat4 mat4Mul(const mat4& m1, const mat4& m2)
{
#ifdef RS_MATH_SIMD_IMPL
    mat4 m;
	m.wide[0] = _mm_add_ps(
	_mm_add_ps(_mm_mul_ps(m1.wide[0], _mm_set1_ps(m2.data[0])),
	_mm_mul_ps(m1.wide[1], _mm_set1_ps(m2.data[1]))),
	_mm_add_ps(_mm_mul_ps(m1.wide[2], _mm_set1_ps(m2.data[2])),
	_mm_mul_ps(m1.wide[3], _mm_set1_ps(m2.data[3])))
	);
	m.wide[1] = _mm_add_ps(
	_mm_add_ps(_mm_mul_ps(m1.wide[0], _mm_set1_ps(m2.data[4])),
	_mm_mul_ps(m1.wide[1], _mm_set1_ps(m2.data[5]))),
	_mm_add_ps(_mm_mul_ps(m1.wide[2], _mm_set1_ps(m2.data[6])),
	_mm_mul_ps(m1.wide[3], _mm_set1_ps(m2.data[7])))
	);
	m.wide[2] = _mm_add_ps(
	_mm_add_ps(_mm_mul_ps(m1.wide[0], _mm_set1_ps(m2.data[8])),
	_mm_mul_ps(m1.wide[1], _mm_set1_ps(m2.data[9]))),
	_mm_add_ps(_mm_mul_ps(m1.wide[2], _mm_set1_ps(m2.data[10])),
	_mm_mul_ps(m1.wide[3], _mm_set1_ps(m2.data[11])))
	);
	m.wide[3] = _mm_add_ps(
	_mm_add_ps(_mm_mul_ps(m1.wide[0], _mm_set1_ps(m2.data[12])),
	_mm_mul_ps(m1.wide[1], _mm_set1_ps(m2.data[13]))),
	_mm_add_ps(_mm_mul_ps(m1.wide[2], _mm_set1_ps(m2.data[14])),
	_mm_mul_ps(m1.wide[3], _mm_set1_ps(m2.data[15])))
	);
	return m;
#else
    mat4 m;
	f32* md = m.data;
	const f32* md1 = m1.data;
	const f32* md2 = m2.data;

	md[0] = md1[0] * md2[0] + md1[4] * md2[1] + md1[8] * md2[2] + md1[12] * md2[3];
	md[1] = md1[1] * md2[0] + md1[5] * md2[1] + md1[9] * md2[2] + md1[13] * md2[3];
	md[2] = md1[2] * md2[0] + md1[6] * md2[1] + md1[10] * md2[2] + md1[14] * md2[3];
	md[3] = md1[3] * md2[0] + md1[7] * md2[1] + md1[11] * md2[2] + md1[15] * md2[3];

	md[4] = md1[0] * md2[4] + md1[4] * md2[5] + md1[8] * md2[6] + md1[12] * md2[7];
	md[5] = md1[1] * md2[4] + md1[5] * md2[5] + md1[9] * md2[6] + md1[13] * md2[7];
	md[6] = md1[2] * md2[4] + md1[6] * md2[5] + md1[10] * md2[6] + md1[14] * md2[7];
	md[7] = md1[3] * md2[4] + md1[7] * md2[5] + md1[11] * md2[6] + md1[15] * md2[7];

	md[8] = md1[0] * md2[8] + md1[4] * md2[9] + md1[8] * md2[10] + md1[12] * md2[11];
	md[9] = md1[1] * md2[8] + md1[5] * md2[9] + md1[9] * md2[10] + md1[13] * md2[11];
	md[10] = md1[2] * md2[8] + md1[6] * md2[9] + md1[10] * md2[10] + md1[14] * md2[11];
	md[11] = md1[3] * md2[8] + md1[7] * md2[9] + md1[11] * md2[10] + md1[15] * md2[11];

	md[12] = md1[0] * md2[12] + md1[4] * md2[13] + md1[8] * md2[14] + md1[12] * md2[15];
	md[13] = md1[1] * md2[12] + md1[5] * md2[13] + md1[9] * md2[14] + md1[13] * md2[15];
	md[14] = md1[2] * md2[12] + md1[6] * md2[13] + md1[10] * md2[14] + md1[14] * md2[15];
	md[15] = md1[3] * md2[12] + md1[7] * md2[13] + md1[11] * md2[14] + md1[15] * md2[15];

	return m;
#endif
}

inline mat4 mat4Inv(mat4 mat) {
    mat4 tmp;
	f32* td = tmp.data;
	const f32* md = mat.data;

	td[0] = md[5]  * md[10] * md[15] -
			md[5]  * md[11] * md[14] -
			md[9]  * md[6]  * md[15] +
			md[9]  * md[7]  * md[14] +
			md[13] * md[6]  * md[11] -
			md[13] * md[7]  * md[10];

	td[4] = -md[4]  * md[10] * md[15] +
			md[4]  * md[11] * md[14] +
			md[8]  * md[6]  * md[15] -
			md[8]  * md[7]  * md[14] -
			md[12] * md[6]  * md[11] +
			md[12] * md[7]  * md[10];

	td[8] = md[4]  * md[9] * md[15] -
			md[4]  * md[11] * md[13] -
			md[8]  * md[5] * md[15] +
			md[8]  * md[7] * md[13] +
			md[12] * md[5] * md[11] -
			md[12] * md[7] * md[9];

	td[12] = -md[4]  * md[9] * md[14] +
			md[4]  * md[10] * md[13] +
			md[8]  * md[5] * md[14] -
			md[8]  * md[6] * md[13] -
			md[12] * md[5] * md[10] +
			md[12] * md[6] * md[9];

	td[1] = -md[1]  * md[10] * md[15] +
			md[1]  * md[11] * md[14] +
			md[9]  * md[2] * md[15] -
			md[9]  * md[3] * md[14] -
			md[13] * md[2] * md[11] +
			md[13] * md[3] * md[10];

	td[5] = md[0]  * md[10] * md[15] -
			md[0]  * md[11] * md[14] -
			md[8]  * md[2] * md[15] +
			md[8]  * md[3] * md[14] +
			md[12] * md[2] * md[11] -
			md[12] * md[3] * md[10];

	td[9] = -md[0]  * md[9] * md[15] +
			md[0]  * md[11] * md[13] +
			md[8]  * md[1] * md[15] -
			md[8]  * md[3] * md[13] -
			md[12] * md[1] * md[11] +
			md[12] * md[3] * md[9];

	td[13] = md[0]  * md[9] * md[14] -
			md[0]  * md[10] * md[13] -
			md[8]  * md[1] * md[14] +
			md[8]  * md[2] * md[13] +
			md[12] * md[1] * md[10] -
			md[12] * md[2] * md[9];

	td[2] = md[1]  * md[6] * md[15] -
			md[1]  * md[7] * md[14] -
			md[5]  * md[2] * md[15] +
			md[5]  * md[3] * md[14] +
			md[13] * md[2] * md[7] -
			md[13] * md[3] * md[6];

	td[6] = -md[0]  * md[6] * md[15] +
			md[0]  * md[7] * md[14] +
			md[4]  * md[2] * md[15] -
			md[4]  * md[3] * md[14] -
			md[12] * md[2] * md[7] +
			md[12] * md[3] * md[6];

	td[10] = md[0]  * md[5] * md[15] -
			md[0]  * md[7] * md[13] -
			md[4]  * md[1] * md[15] +
			md[4]  * md[3] * md[13] +
			md[12] * md[1] * md[7] -
			md[12] * md[3] * md[5];

	td[14] = -md[0]  * md[5] * md[14] +
			md[0]  * md[6] * md[13] +
			md[4]  * md[1] * md[14] -
			md[4]  * md[2] * md[13] -
			md[12] * md[1] * md[6] +
			md[12] * md[2] * md[5];

	td[3] = -md[1] * md[6] * md[11] +
			md[1] * md[7] * md[10] +
			md[5] * md[2] * md[11] -
			md[5] * md[3] * md[10] -
			md[9] * md[2] * md[7] +
			md[9] * md[3] * md[6];

	td[7] = md[0] * md[6] * md[11] -
			md[0] * md[7] * md[10] -
			md[4] * md[2] * md[11] +
			md[4] * md[3] * md[10] +
			md[8] * md[2] * md[7] -
			md[8] * md[3] * md[6];

	td[11] = -md[0] * md[5] * md[11] +
			md[0] * md[7] * md[9] +
			md[4] * md[1] * md[11] -
			md[4] * md[3] * md[9] -
			md[8] * md[1] * md[7] +
			md[8] * md[3] * md[5];

	td[15] = md[0] * md[5] * md[10] -
			md[0] * md[6] * md[9] -
			md[4] * md[1] * md[10] +
			md[4] * md[2] * md[9] +
			md[8] * md[1] * md[6] -
			md[8] * md[2] * md[5];

	f32 det = md[0] * td[0] + md[1] * td[4] + md[2] * td[8] +
			md[3] * td[12];

	if(det == 0) {
        return mat4Identity();
	}

	det = 1.f / det;

    mat4 out;
	for(u32 i = 0; i < 16; ++i) {
		out.data[i] = td[i] * det;
	}

	return out;
}

inline vec3f vec3fMulMat4(vec3f vec, mat4 mat) {
    vec3f o;
	f32* md = mat.data;
	o.x = vec.x * md[0] + vec.y * md[4] + vec.z * md[8] + md[12];
	o.y = vec.x * md[1] + vec.y * md[5] + vec.z * md[9] + md[13];
	o.z = vec.x * md[2] + vec.y * md[6] + vec.z * md[10] + md[14];
	return o;
}

inline vec4f vec4fMulMat4(vec4f vec, mat4 mat) {
    vec4f o;
	f32* md = mat.data;
	o.x = vec.x * md[0] + vec.y * md[4] + vec.z * md[8] + vec.w * md[12];
	o.y = vec.x * md[1] + vec.y * md[5] + vec.z * md[9] + vec.w * md[13];
	o.z = vec.x * md[2] + vec.y * md[6] + vec.z * md[10] + vec.w * md[14];
	o.w = vec.x * md[3] + vec.y * md[7] + vec.z * md[11] + vec.w * md[15];
	return o;
}

inline mat4 mat4Translate(const vec3f& t)
{
    return mat4{
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		t.x, t.y, t.z, 1.f
	};
}

inline mat4 mat4Scale(const vec3f& s)
{
    return mat4{
		s.x, 0.f, 0.f, 0.f,
		0.f, s.y, 0.f, 0.f,
		0.f, 0.f, s.z, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

inline mat4 mat4RotateAxisX(f32 angle)
{
    return mat4{
		1.f, 0.f, 0.f, 0.f,
		0.f, cosf(angle), -sinf(angle), 0.f,
		0.f, sinf(angle), cosf(angle), 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

inline mat4 mat4RotateAxisY(f32 angle)
{
    return mat4{
		cosf(angle), 0.f, sinf(angle), 0.f,
		0.f, 1.f, 0.f, 0.f,
		-sinf(angle), 0.f, cosf(angle), 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

inline mat4 mat4RotateAxisZ(f32 angle)
{
    return mat4{
		cosf(angle), -sinf(angle), 0.f, 0.f,
		sinf(angle), cosf(angle), 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

inline mat4 mat4Perspective(f32 fov, f32 aspect, f32 nearPlane, f32 farPlane)
{
	fov *= 0.5f;
	f32 d = farPlane - nearPlane;
	f32 cosFov = cosf(fov);
	f32 sinFov = sinf(fov);

	if(d == 0 || sinFov == 0 || aspect == 0) {
        return mat4Identity();
	}

	f32 cotangent = cosFov / sinFov;

    mat4 m = mat4Identity();
	m.data[0] = cotangent / aspect;
	m.data[5] = cotangent;
	m.data[10] = -(farPlane + nearPlane) / d;
	m.data[11] = -1.f;
	m.data[14] = -2.f * nearPlane * farPlane / d;
	m.data[15] = 0.f;
	return m;
}

inline mat4 mat4Orthographic(f32 left, f32 right, f32 bottom, f32 top,
									 f32 nearPlane, f32 farPlane)
{
    mat4 m = mat4Identity();
	m.data[0] = 2.f / (right - left);
	m.data[5] = 2.f / (top - bottom);
	m.data[10] = -2.f / (farPlane - nearPlane);
	m.data[12] = -((right + left) / (right - left));
	m.data[13] = -((top + bottom) / (top - bottom));
	m.data[14] = -((farPlane + nearPlane) / (farPlane - nearPlane));
	return m;
}

inline mat4 mat4LookAt(const vec3f& eye, const vec3f& center, const vec3f& up)
{
    vec3f dir = rs_normalize(vec3fSub(center, eye));
    vec3f c0 = rs_normalize(rs_cross(dir, up));
    vec3f c1 = rs_normalize(rs_cross(c0, dir));

    return mat4{
		c0.x,
		c1.x,
		-dir.x,
		0.f,

		c0.y,
		c1.y,
		-dir.y,
		0.f,

		c0.z,
		c1.z,
		-dir.z,
		0.f,

		-rs_dot(c0, eye),
		-rs_dot(c1, eye),
		rs_dot(dir, eye),
		1.f
	};
}

#ifdef RS_MATH_OPERATORS
inline mat4 operator*(const mat4& m1, const mat4& m2) {
    return mat4Mul(m1, m2);
}

inline vec3f operator*(const vec3f& v, const mat4& m) {
    return vec3fMulMat4(v, m);
}
#endif

union alignas(16) rs_Quat
{
	struct { f32 x, y, z, w; };
	f32 data[4];
#ifdef RS_MATH_SIMD_IMPL
	__m128 wide;
#endif

	constexpr rs_Quat(): data{0.f, 0.f, 0.f, 1.f}{}

	constexpr rs_Quat(f32 x_, f32 y_, f32 z_, f32 w_)
	: data{x_, y_, z_, w_} {
	}
};

inline rs_Quat rs_normalize(rs_Quat quat) {
	f32 lengthInv = 1.f / sqrtf(quat.x * quat.x + quat.y * quat.y + quat.z * quat.z + quat.w * quat.w);
	quat.x *= lengthInv;
	quat.y *= lengthInv;
	quat.z *= lengthInv;
	quat.w *= lengthInv;
	return quat;
}

inline bool rs_QuatIsNull(const rs_Quat& quat) {
	return (quat.x == 0 && quat.y == 0 && quat.z && quat.w == 1.f);
}

inline rs_Quat rs_QuatConjugate(rs_Quat q) {
	return {-q.x, -q.y, -q.z, q.w};
}

inline rs_Quat rs_QuatAxisRotation(vec3f axis, f32 angle) {
	rs_Quat quat;
	f32 sa = sinf(angle * 0.5f);
	quat.x = axis.x * sa;
	quat.y = axis.y * sa;
	quat.z = axis.z * sa;
	quat.w = cosf(angle * 0.5f);
	quat = rs_normalize(quat);
	return quat;
}

inline rs_Quat rs_QuatGetRotVec3(vec3f v1, vec3f v2) {
	rs_Quat quat;
    vec3f c = rs_cross(v1, v2);
	quat.x = c.x;
	quat.y = c.y;
	quat.z = c.z;
	quat.w = sqrtf(rs_lengthSq(v1) * rs_lengthSq(v2)) + rs_dot(v1, v2);
	return rs_normalize(quat);
}

inline mat4 rs_QuatMatrix(const rs_Quat& quat) {
	f32 xx = quat.x * quat.x;
	f32 xy = quat.x * quat.y;
	f32 xz = quat.x * quat.z;
	f32 xw = quat.x * quat.w;

	f32 yy = quat.y * quat.y;
	f32 yz = quat.y * quat.z;
	f32 yw = quat.y * quat.w;

	f32 zz = quat.z * quat.z;
	f32 zw = quat.z * quat.w;

    return mat4{
		1.f - 2.f * (yy + zz),
		2.f * (xy + zw),
		2.f * (xz - yw),
		0,

		2.f * (xy - zw),
		1.f - 2.f * (xx + zz),
		2.f * (yz + xw),
		0,

		2.f * (xz + yw),
		2.f * (yz - xw),
		1.f - 2.f * (xx + yy),
		0,

		0,
		0,
		0,
		1,
	};
}

inline rs_Quat rs_QuatMul(rs_Quat qa, rs_Quat qb) {
	// Grassman product (equivalent methods)
#if 0
    vec3f qav{qa.x, qa.y, qa.z};
    vec3f qbv{qb.x, qb.y, qb.z};
    vec3f qv = vec3fAdd(vec3fAdd(vec3fMul(qbv, qa.w), vec3fMul(qav, qb.w)),
							  rs_cross(qav, qbv));
	return {qv.x, qv.y, qv.z, qa.w * qb.w - rs_dot(qav, qbv)};
#endif
	rs_Quat quat;
	quat.x = qa.w * qb.x + qa.x * qb.w + qa.y * qb.z - qa.z * qb.y;
	quat.y = qa.w * qb.y + qa.y * qb.w + qa.z * qb.x - qa.x * qb.z;
	quat.z = qa.w * qb.z + qa.z * qb.w + qa.x * qb.y - qa.y * qb.x;
	quat.w = qa.w * qb.w - qa.x * qb.x - qa.y * qb.y - qa.z * qb.z;
	return quat;
}

#ifdef RS_MATH_OPERATORS
inline rs_Quat operator*(const rs_Quat& q1, const rs_Quat& q2) {
	return rs_QuatMul(q1, q2);
}

inline rs_Quat& operator*=(rs_Quat& q, const rs_Quat& other) {
	return q = rs_QuatMul(q, other);
}
#endif

inline vec3f rs_QuatRotateVec3(vec3f vec, rs_Quat quat) {
	rs_Quat qvec = {vec.x, vec.y, vec.z, 0};
	qvec = rs_QuatMul(rs_QuatMul(quat, qvec), rs_QuatConjugate(quat));
	return {qvec.x, qvec.y, qvec.z};
}


struct rs_AABB2
{
    vec2f min = {0,0}, max = {0,0};
};

inline bool rs_AABB2_intersect(const rs_AABB2& A, const rs_AABB2& B) {
	f32 d0 = B.max.x - A.min.x;
	f32 d1 = B.min.x - A.max.x;

	// no overlap
	if(d0 < 0.0f || d1 > 0.0f) {
		return false;
	}

	// y axis ------------
	d0 = B.max.y - A.min.y;
	d1 = B.min.y - A.max.y;

	// no overlap
	if(d0 < 0.0f || d1 > 0.0f) {
		return false;
	}

	return true;
}

struct rs_AABB3
{
    vec3f min, max;
};

struct rs_Sphere
{
    vec3f center = {0, 0, 0};
	f32 radius = 0;
};
