/**
 * @file Vec3.cpp
 * @author RT Team - Dev A
 * @date 2026-04-01
 * @brief 3D vector implementation
 */

#include "core/Vec3.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <immintrin.h>
#include <stdexcept>

// Helper: load two doubles starting at address p into an SSE register
static inline __m128d load2(const double* p)
{
    return _mm_loadu_pd(p);
}

// Helper: store two doubles from an SSE register to address p
static inline void store2(double* p, __m128d v)
{
    _mm_storeu_pd(p, v);
}

// Constructor implementations
Vec3::Vec3() : x(0.0), y(0.0), z(0.0) {}

Vec3::Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

Vec3::Vec3(const Vec3& other) : x(other.x), y(other.y), z(other.z) {}

Vec3& Vec3::operator=(const Vec3& other)
{
    if (this == &other) return *this;
    x = other.x;
    y = other.y;
    z = other.z;
    return *this;
}

// Arithmetic operators (SSE-optimised hot-path)
Vec3 Vec3::operator+(const Vec3& other) const
{
    Vec3 r;
    store2(&r.x, _mm_add_pd(load2(&x), load2(&other.x)));
    r.z = z + other.z;
    return r;
}

Vec3 Vec3::operator-(const Vec3& other) const
{
    Vec3 r;
    store2(&r.x, _mm_sub_pd(load2(&x), load2(&other.x)));
    r.z = z - other.z;
    return r;
}

Vec3 Vec3::operator-() const
{
    // XOR with sign bit to flip all components
    const __m128d sign = _mm_set1_pd(-0.0); // 0x8000000000000000 as double
    __m128d xy = _mm_xor_pd(load2(&x), sign);
    Vec3 r;
    store2(&r.x, xy);
    r.z = -z;
    return r;
}

Vec3 Vec3::operator*(double scalar) const
{
    __m128d s = _mm_set1_pd(scalar);
    Vec3 r;
    store2(&r.x, _mm_mul_pd(load2(&x), s));
    r.z = z * scalar;
    return r;
}

Vec3 Vec3::operator/(double scalar) const
{
    if (std::abs(scalar) <= EPSILON) throw std::runtime_error("Division by zero in Vec3::operator/");
    __m128d s = _mm_set1_pd(scalar);
    Vec3 r;
    store2(&r.x, _mm_div_pd(load2(&x), s));
    r.z = z / scalar;
    return r;
}

Vec3& Vec3::operator+=(const Vec3& other)
{
    store2(&x, _mm_add_pd(load2(&x), load2(&other.x)));
    z += other.z;
    return *this;
}

Vec3& Vec3::operator-=(const Vec3& other)
{
    store2(&x, _mm_sub_pd(load2(&x), load2(&other.x)));
    z -= other.z;
    return *this;
}

Vec3& Vec3::operator*=(double scalar)
{
    __m128d s = _mm_set1_pd(scalar);
    store2(&x, _mm_mul_pd(load2(&x), s));
    z *= scalar;
    return *this;
}

// Comparison operators
bool Vec3::operator==(const Vec3& other) const
{
    return (std::abs(x - other.x) <= EPSILON) &&
           (std::abs(y - other.y) <= EPSILON) &&
           (std::abs(z - other.z) <= EPSILON);
}

bool Vec3::operator!=(const Vec3& other) const
{
    return !(*this == other);
}

// Vector operations (SSE-optimised hot-path)
Vec3 Vec3::componentMult(const Vec3& other) const
{
    Vec3 r;
    store2(&r.x, _mm_mul_pd(load2(&x), load2(&other.x)));
    r.z = z * other.z;
    return r;
}

double Vec3::dot(const Vec3& other) const
{
    // Dot product of (x,y) using SSE2 (no SSE3 needed), then add z*z
    __m128d xy = _mm_mul_pd(load2(&x), load2(&other.x));       // (x1*x2, y1*y2)
    __m128d yx = _mm_shuffle_pd(xy, xy, 0x01);                 // (y1*y2, x1*x2)
    __m128d sum = _mm_add_pd(xy, yx);                          // (x1*x2+y1*y2, same)
    return _mm_cvtsd_f64(sum) + z * other.z;
}

Vec3 Vec3::cross(const Vec3& other) const
{
    // Cross product doesn't map well to 2-wide SIMD; keep scalar
    return Vec3(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
}

double Vec3::magnitude() const
{
    return std::sqrt(magnitudeSquared());
}

double Vec3::magnitudeSquared() const
{
    return dot(*this);
}

Vec3& Vec3::normalize()
{
    double mag = magnitude();
    if (mag <= EPSILON) throw std::runtime_error("Cannot normalize zero-length vector");
    double invMag = 1.0 / mag;
    __m128d s = _mm_set1_pd(invMag);
    store2(&x, _mm_mul_pd(load2(&x), s));
    z *= invMag;
    return *this;
}

Vec3 Vec3::normalized() const
{
    double mag = magnitude();
    if (mag <= EPSILON) throw std::runtime_error("Cannot normalize zero-length vector");
    return *this * (1.0 / mag);
}

double Vec3::distance(const Vec3& other) const
{
    return (*this - other).magnitude();
}

double Vec3::distanceSquared(const Vec3& other) const
{
    return (*this - other).magnitudeSquared();
}

Vec3 Vec3::lerp(const Vec3& other, double t) const
{
    return (*this) * (1.0 - t) + other * t;
}

Vec3 Vec3::reflect(const Vec3& normal) const
{
    // r = v - 2 (v·n) n
    return (*this) - normal * (2.0 * this->dot(normal));
}

bool Vec3::refract(const Vec3& normal, double refractionIndex, Vec3& refractedDir) const
{
    // Using Snell's law. Assumes this is the incident vector (pointing away from surface).
    Vec3 uv = this->normalized();
    double dt = uv.dot(normal);
    double discriminant = 1.0 - refractionIndex * refractionIndex * (1.0 - dt * dt);
    if (discriminant > 0.0) {
        refractedDir = (uv - normal * dt) * refractionIndex - normal * std::sqrt(discriminant);
        return true;
    }
    return false;
}

// Stream output
std::ostream& operator<<(std::ostream& out, const Vec3& v)
{
    return out << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}
