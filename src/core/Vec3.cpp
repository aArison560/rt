/**
 * @file Vec3.cpp
 * @author RT Team - Dev A
 * @date 2026-04-01
 * @brief 3D vector implementation
 */

#include "core/Vec3.hpp"
#include <cassert>
#include <cmath>
#include <stdexcept>

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

// Arithmetic operators
Vec3 Vec3::operator+(const Vec3& other) const
{
    return Vec3(x + other.x, y + other.y, z + other.z);
}

Vec3 Vec3::operator-(const Vec3& other) const
{
    return Vec3(x - other.x, y - other.y, z - other.z);
}

Vec3 Vec3::operator-() const
{
    return Vec3(-x, -y, -z);
}

Vec3 Vec3::operator*(double scalar) const
{
    return Vec3(x * scalar, y * scalar, z * scalar);
}

Vec3 Vec3::operator/(double scalar) const
{
    if (std::abs(scalar) <= EPSILON) throw std::runtime_error("Division by zero in Vec3::operator/");
    return Vec3(x / scalar, y / scalar, z / scalar);
}

Vec3& Vec3::operator+=(const Vec3& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vec3& Vec3::operator-=(const Vec3& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Vec3& Vec3::operator*=(double scalar)
{
    x *= scalar;
    y *= scalar;
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

// Vector operations
Vec3 Vec3::componentMult(const Vec3& other) const
{
    return Vec3(x * other.x, y * other.y, z * other.z);
}

double Vec3::dot(const Vec3& other) const
{
    return x * other.x + y * other.y + z * other.z;
}

Vec3 Vec3::cross(const Vec3& other) const
{
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
    return x * x + y * y + z * z;
}

Vec3& Vec3::normalize()
{
    double mag = magnitude();
    if (mag <= EPSILON) throw std::runtime_error("Cannot normalize zero-length vector");
    x /= mag;
    y /= mag;
    z /= mag;
    return *this;
}

Vec3 Vec3::normalized() const
{
    double mag = magnitude();
    if (mag <= EPSILON) throw std::runtime_error("Cannot normalize zero-length vector");
    return Vec3(x / mag, y / mag, z / mag);
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
