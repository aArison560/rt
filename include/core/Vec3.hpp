/**
 * @file Vec3.hpp
 * @author RT Team - Dev A
 * @date 2026-04-01
 * @brief 3D vector class with mathematical operations
 *
 * Provides a 3D vector representation with standard geometric operations
 * including dot product, cross product, normalization, and operator overloading.
 *
 * @relationships
 * - Used by: Ray, Transform, Material, Texture, all AObject subclasses
 * - Foundation for all geometric calculations in the ray tracer
 */

#pragma once

#include <cmath>
#include <iostream>

class Vec3
{
public:
    double x;
    double y;
    double z;

    /**
     * @brief Default constructor - initializes to zero vector
     */
    Vec3();

    /**
     * @brief Constructor with explicit coordinates
     * @param x The X component
     * @param y The Y component
     * @param z The Z component
     */
    Vec3(double x, double y, double z);

    /**
     * @brief Copy constructor
     * @param other The Vec3 to copy
     */
    Vec3(const Vec3& other);

    /**
     * @brief Assignment operator
     * @param other The Vec3 to assign
     * @return Reference to this Vec3
     */
    Vec3& operator=(const Vec3& other);

    /**
     * @brief Vector addition operator
     * @param other The Vec3 to add
     * @return New Vec3 representing the sum
     */
    Vec3 operator+(const Vec3& other) const;

    /**
     * @brief Vector subtraction operator
     * @param other The Vec3 to subtract
     * @return New Vec3 representing the difference
     */
    Vec3 operator-(const Vec3& other) const;

    /**
     * @brief Vector negation operator
     * @return New Vec3 with negated components
     */
    Vec3 operator-() const;

    /**
     * @brief Scalar multiplication operator
     * @param scalar The scalar value to multiply
     * @return New Vec3 with scaled components
     */
    Vec3 operator*(double scalar) const;

    /**
     * @brief Scalar division operator
     * @param scalar The scalar value to divide by
     * @return New Vec3 with divided components
     * @throw std::runtime_error if scalar is zero
     */
    Vec3 operator/(double scalar) const;

    /**
     * @brief Compound addition operator
     * @param other The Vec3 to add
     * @return Reference to this Vec3
     */
    Vec3& operator+=(const Vec3& other);

    /**
     * @brief Compound subtraction operator
     * @param other The Vec3 to subtract
     * @return Reference to this Vec3
     */
    Vec3& operator-=(const Vec3& other);

    /**
     * @brief Compound scalar multiplication operator
     * @param scalar The scalar value to multiply
     * @return Reference to this Vec3
     */
    Vec3& operator*=(double scalar);

    /**
     * @brief Equality comparison operator
     * @param other The Vec3 to compare
     * @return true if vectors are equal within epsilon tolerance
     */
    bool operator==(const Vec3& other) const;

    /**
     * @brief Inequality comparison operator
     * @param other The Vec3 to compare
     * @return true if vectors are not equal within epsilon tolerance
     */
    bool operator!=(const Vec3& other) const;

    /**
     * @brief Component-wise multiplication (Hadamard product)
     * @param other The Vec3 to multiply component-wise
     * @return New Vec3 with component-wise products
     */
    Vec3 componentMult(const Vec3& other) const;

    /**
     * @brief Dot product calculation
     * @param other The Vec3 to dot with
     * @return Scalar dot product result
     */
    double dot(const Vec3& other) const;

    /**
     * @brief Cross product calculation
     * @param other The Vec3 to cross with
     * @return New Vec3 representing the cross product
     */
    Vec3 cross(const Vec3& other) const;

    /**
     * @brief Calculate vector magnitude (length)
     * @return The magnitude of the vector
     */
    double magnitude() const;

    /**
     * @brief Calculate squared magnitude (avoids sqrt)
     * @return The squared magnitude
     */
    double magnitudeSquared() const;

    /**
     * @brief Normalize the vector to unit length
     * @return Reference to this Vec3
     * @throw std::runtime_error if vector is zero
     */
    Vec3& normalize();

    /**
     * @brief Get a normalized copy without modifying this vector
     * @return New normalized Vec3
     * @throw std::runtime_error if vector is zero
     */
    Vec3 normalized() const;

    /**
     * @brief Calculate distance to another point
     * @param other The other point as Vec3
     * @return The distance between points
     */
    double distance(const Vec3& other) const;

    /**
     * @brief Calculate squared distance (avoids sqrt)
     * @param other The other point as Vec3
     * @return The squared distance
     */
    double distanceSquared(const Vec3& other) const;

    /**
     * @brief Linearly interpolate between two vectors
     * @param other The target vector
     * @param t Interpolation parameter (0.0 to 1.0)
     * @return New interpolated Vec3
     */
    Vec3 lerp(const Vec3& other, double t) const;

    /**
     * @brief Reflect vector around a normal
     * @param normal The surface normal
     * @return New reflected Vec3
     */
    Vec3 reflect(const Vec3& normal) const;

    /**
     * @brief Refract vector through a surface
     * @param normal The surface normal
     * @param refractionIndex The refractive index ratio (n1/n2)
     * @param refractedDir Output vector for refracted direction
     * @return true if refraction occurs, false if total internal reflection
     */
    bool refract(const Vec3& normal, double refractionIndex, Vec3& refractedDir) const;

    /**
     * @brief Stream output operator
     * @param out The output stream
     * @param v The Vec3 to output
     * @return Reference to the output stream
     */
    friend std::ostream& operator<<(std::ostream& out, const Vec3& v);

    /**
     * @brief Constant for epsilon comparisons
     */
    static constexpr double EPSILON = 1e-6;
};

/**
 * @brief Scalar * Vec3 operator (for commutative property)
 * @param scalar The scalar value
 * @param v The Vec3 to multiply
 * @return New Vec3 with scaled components
 */
inline Vec3 operator*(double scalar, const Vec3& v)
{
    return v * scalar;
}
