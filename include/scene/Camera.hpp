/**
 * @file Camera.hpp
 * @author RT Team - Dev A
 * @date 2026-04-01
 * @brief Camera class for ray generation from screen coordinates
 *
 * Represents a camera in 3D space with position, direction, and field of view.
 * Generates rays for ray tracing by converting screen coordinates to world space rays.
 *
 * @relationships
 * - Composed of: Transform, Vec3 (position, direction, up vector)
 * - Produces: Ray objects from screen coordinates
 * - Used by: Renderer (generates rays for tracing)
 * - Similar to OpenGL camera model
 */

#pragma once

#include "Transform.hpp"
#include "core/Ray.hpp"

class Camera
{
public:
    /**
     * @brief Default constructor - creates camera at origin looking down positive Z
     */
    Camera();

    /**
     * @brief Constructor with explicit parameters
     * @param position Camera position in world space
     * @param direction Camera look direction (will be normalized)
     * @param upVector Up vector for camera orientation (will be normalized)
     * @param fieldOfViewDegrees Field of view in degrees (vertical)
     */
    Camera(const Vec3& position, const Vec3& direction, const Vec3& upVector,
           double fieldOfViewDegrees);

    /**
     * @brief Copy constructor
     * @param other The Camera to copy
     */
    Camera(const Camera& other);

    /**
     * @brief Assignment operator
     * @param other The Camera to assign
     * @return Reference to this Camera
     */
    Camera& operator=(const Camera& other);

    /**
     * @brief Generate a ray from a screen coordinate
     * @param screenX X coordinate on screen (0 to width-1)
     * @param screenY Y coordinate on screen (0 to height-1)
     * @param screenWidth Total screen width in pixels
     * @param screenHeight Total screen height in pixels
     * @return Ray from camera through the screen pixel
     */
    Ray generateRay(double screenX, double screenY, int screenWidth, int screenHeight) const;

    /**
     * @brief Set camera position
     * @param position The new position
     */
    void setPosition(const Vec3& position);

    /**
     * @brief Set camera look direction
     * @param direction The new direction (will be normalized)
     */
    void setDirection(const Vec3& direction);

    /**
     * @brief Set camera up vector
     * @param upVector The new up vector (will be normalized)
     */
    void setUp(const Vec3& upVector);

    /**
     * @brief Set field of view
     * @param fieldOfViewDegrees Field of view in degrees (vertical)
     */
    void setFOV(double fieldOfViewDegrees);

    /**
     * @brief Get camera position
     * @return The position as Vec3
     */
    const Vec3& getPosition() const;

    /**
     * @brief Get camera direction
     * @return The direction as Vec3
     */
    const Vec3& getDirection() const;

    /**
     * @brief Get camera up vector
     * @return The up vector as Vec3
     */
    const Vec3& getUp() const;

    /**
     * @brief Get right vector (cross product of direction and up)
     * @return The right vector as Vec3
     */
    const Vec3& getRight() const;

    /**
     * @brief Get field of view in degrees
     * @return FOV in degrees
     */
    double getFOV() const;

    /**
     * @brief Move camera forward
     * @param distance Distance to move
     */
    void moveForward(double distance);

    /**
     * @brief Move camera backward
     * @param distance Distance to move
     */
    void moveBackward(double distance);

    /**
     * @brief Move camera left
     * @param distance Distance to move
     */
    void moveLeft(double distance);

    /**
     * @brief Move camera right
     * @param distance Distance to move
     */
    void moveRight(double distance);

    /**
     * @brief Move camera up
     * @param distance Distance to move
     */
    void moveUp(double distance);

    /**
     * @brief Move camera down
     * @param distance Distance to move
     */
    void moveDown(double distance);

    /**
     * @brief Rotate camera around local X axis (pitch)
     * @param angleRadians Rotation angle in radians
     */
    void rotatePitch(double angleRadians);

    /**
     * @brief Rotate camera around local Y axis (yaw)
     * @param angleRadians Rotation angle in radians
     */
    void rotateYaw(double angleRadians);

    /**
     * @brief Rotate camera around local Z axis (roll)
     * @param angleRadians Rotation angle in radians
     */
    void rotateRoll(double angleRadians);

    /**
     * @brief Update cached camera basis vectors after modification
     */
    void updateBasis();

private:
    Vec3 position;           ///< Camera position in world space
    Vec3 direction;          ///< Camera look direction (normalized)
    Vec3 upVector;           ///< Camera up vector (normalized)
    Vec3 rightVector;        ///< Camera right vector (cached, normalized)
    double fieldOfViewDegrees; ///< Vertical field of view in degrees

    /**
     * @brief Recompute the right and recalibrate up vector for consistency
     */
    void recalculateBasis();
};
