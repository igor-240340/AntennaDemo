#pragma once

#include "Vec4f.h"

class Vec3f {
public:
    float x;
    float y;
    float z;

public:
    static const Vec3f zero;
    static const Vec3f up;

public:
    Vec3f() = default;
    ~Vec3f() = default;
    Vec3f(float x, float y, float z);
    Vec3f(Vec4f vec4);

    float length() const;
    float length_squared() const;
    Vec3f get_normalized() const;

    Vec3f operator/(float scalar) const;
    Vec3f operator+(const Vec3f& b) const;
    Vec3f operator-(const Vec3f& b) const;
    Vec3f operator*(float scalar) const;
    Vec3f operator-() const;

    friend Vec3f operator*(float scalar, const Vec3f& vec);

    static float dot(const Vec3f& a, const Vec3f& b);
    static Vec3f cross(const Vec3f& a, const Vec3f& b);
};
