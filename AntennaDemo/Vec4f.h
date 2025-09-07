#pragma once

class Vec3f;

class Vec4f {
public:
    float x;
    float y;
    float z;
    float w;

public:
    Vec4f() = default;
    ~Vec4f() = default;
    Vec4f(const Vec3f& vec3, float w = 1.0f);
    Vec4f(float x, float y, float z, float w = 1.0f);

    Vec4f operator/(float scalar) const;

    static float dot(const Vec4f& a, const Vec4f& b);
};
