#include "Vec4f.h"
#include "Vec3f.h"

Vec4f::Vec4f(const Vec3f& vec3, float w) : x(vec3.x), y(vec3.y), z(vec3.z), w(w) {
}

Vec4f::Vec4f(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {
}

// NOTE: Допустим, что scalar = w.
// Тогда возможна ситуация, при которой w * scalar_inv не даст в точности единицу.
Vec4f Vec4f::operator/(float scalar) const {
    const float scalar_inv = 1.0f / scalar;
    return Vec4f(x * scalar_inv, y * scalar_inv, z * scalar_inv, w * scalar_inv);
}

float Vec4f::dot(const Vec4f& a, const Vec4f& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
