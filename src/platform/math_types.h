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
