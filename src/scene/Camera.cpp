/**
 * @file Camera.cpp
 * @author RT Team - Dev A
 * @date 2026-04-01
 * @brief Camera implementation
 */

#include "scene/Camera.hpp"
#include <cmath>

Camera::Camera() 
    : position(0, 0, 0), direction(0, 0, 1), upVector(0, 1, 0), 
      rightVector(1, 0, 0), fieldOfViewDegrees(45.0)
{
    // Default camera at origin looking down positive Z
    recalculateBasis();
}

Camera::Camera(const Vec3& position, const Vec3& direction, const Vec3& upVector,
               double fieldOfViewDegrees)
    : position(position), direction(direction), upVector(upVector),
      fieldOfViewDegrees(fieldOfViewDegrees)
{
    recalculateBasis();
}

Camera::Camera(const Camera& other)
    : position(other.position), direction(other.direction), upVector(other.upVector),
      rightVector(other.rightVector), fieldOfViewDegrees(other.fieldOfViewDegrees) {}

Camera& Camera::operator=(const Camera& other)
{
    if (this == &other) return *this;
    position = other.position;
    direction = other.direction;
    upVector = other.upVector;
    rightVector = other.rightVector;
    fieldOfViewDegrees = other.fieldOfViewDegrees;
    return *this;
}

Ray Camera::generateRay(double screenX, double screenY, int screenWidth, int screenHeight) const
{
    // Convert pixel center to NDC
    double nx = (screenX + 0.5) / static_cast<double>(screenWidth);
    double ny = (screenY + 0.5) / static_cast<double>(screenHeight);

    double aspect = static_cast<double>(screenWidth) / static_cast<double>(screenHeight);
    double fovRad = fieldOfViewDegrees * M_PI / 180.0;
    double scale = std::tan(fovRad * 0.5);

    double px = (2.0 * nx - 1.0) * aspect * scale;
    double py = (1.0 - 2.0 * ny) * scale; // y flipped so +y is up

    Vec3 rayDir = direction + rightVector * px + upVector * py;
    try {
        rayDir = rayDir.normalized();
    } catch (...) {
        rayDir = direction;
    }
    return Ray(position, rayDir);
}

void Camera::setPosition(const Vec3& pos)
{
    position = pos;
}

void Camera::setDirection(const Vec3& dir)
{
    direction = dir;
    recalculateBasis();
}

void Camera::setUp(const Vec3& up)
{
    upVector = up;
    recalculateBasis();
}

void Camera::setFOV(double fov)
{
    fieldOfViewDegrees = fov;
}

const Vec3& Camera::getPosition() const
{
    return position;
}

const Vec3& Camera::getDirection() const
{
    return direction;
}

const Vec3& Camera::getUp() const
{
    return upVector;
}

const Vec3& Camera::getRight() const
{
    return rightVector;
}

double Camera::getFOV() const
{
    return fieldOfViewDegrees;
}

void Camera::moveForward(double distance)
{
    position += direction * distance;
}

void Camera::moveBackward(double distance)
{
    position -= direction * distance;
}

void Camera::moveLeft(double distance)
{
    position -= rightVector * distance;
}

void Camera::moveRight(double distance)
{
    position += rightVector * distance;
}

void Camera::moveUp(double distance)
{
    position += upVector * distance;
}

void Camera::moveDown(double distance)
{
    position -= upVector * distance;
}

void Camera::rotatePitch(double angleRadians)
{
    (void)angleRadians;
}

void Camera::rotateYaw(double angleRadians)
{
    (void)angleRadians;
}

void Camera::rotateRoll(double angleRadians)
{
    (void)angleRadians;
}

void Camera::updateBasis()
{
    recalculateBasis();
}

void Camera::recalculateBasis()
{
    try {
        direction = direction.normalized();
    } catch (...) {
        direction = Vec3(0,0,1);
    }
    try {
        upVector = upVector.normalized();
    } catch (...) {
        upVector = Vec3(0,1,0);
    }

    // right = direction x up
    rightVector = direction.cross(upVector);
    try {
        rightVector = rightVector.normalized();
    } catch (...) {
        rightVector = Vec3(1,0,0);
    }

    // Re-orthogonalize up
    upVector = rightVector.cross(direction);
    try {
        upVector = upVector.normalized();
    } catch (...) {
        upVector = Vec3(0,1,0);
    }
}
