#pragma once

#include <cstdint>

namespace platform {

/// A direction, extent, or gravity vector. Single precision is ample.
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    friend bool operator==(const Vec3&, const Vec3&) = default;
};

/// A world-space position.
///
/// @remarks
/// Double precision, mirroring Box3D's own split: translation is double, and
/// rotation stays float because it never needs the extra range. Narrowing this
/// would silently cost precision far from the origin, which is exactly what
/// large levels produce.
struct Point3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    friend bool operator==(const Point3&, const Point3&) = default;
};

/// A 2D vector: screen positions, sizes, and texture coordinates.
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    friend bool operator==(const Vec2&, const Vec2&) = default;
};

/// A rotation.
///
/// @remarks
/// A quaternion rather than Euler angles: no gimbal lock, and it interpolates
/// cleanly, which matters because presentation blends between simulation states.
/// Identity is `{0, 0, 0, 1}`.
struct Quat {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    friend bool operator==(const Quat&, const Quat&) = default;
};

/// Where and how something sits in the world.
struct Transform {
    Point3 position;
    Quat rotation;
    Vec3 scale{ 1.0f, 1.0f, 1.0f };

    friend bool operator==(const Transform&, const Transform&) = default;
};

/// A rectangle, used for texture atlas sub-regions and screen-space areas.
///
/// @remarks
/// For UVs the units are **normalised** [0, 1]. For screen space they are
/// pixels. Which one applies is stated by the function taking it.
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    friend bool operator==(const Rect&, const Rect&) = default;
};

/// 8-bit RGBA.
struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    friend bool operator==(const Color&, const Color&) = default;
};

/// A view into the world.
///
/// @remarks
/// The camera is presentation, never simulation. Its position may derive from a
/// simulated body, but nothing in the rules may read it back
/// (presentation_protocol.md).
struct Camera {
    Point3 position;
    Point3 target;
    float fovDegrees = 60.0f;
};

} // namespace platform
