#pragma once

#include <array>

#include "Vec3f.h"
#include "Vec4f.h"

// Column-major order.
class Mat4f {
public:
    Mat4f();
    ~Mat4f() = default;

    Vec4f operator*(const Vec4f& vec) const;
    Mat4f operator*(const Mat4f& vec) const;

    static Mat4f create_identity();

    static Mat4f create_look_at(const Vec3f& pos, const Vec3f& look_dir, const Vec3f& up);
    static Mat4f create_perspective(float fov_vert_rad, float aspect_ratio, float near, float far);
    static Mat4f create_ortho(float left, float right, float bottom, float top, float near, float far);
    static Mat4f create_viewport(int w, int h);

    static Mat4f create_translation(const Vec3f& v);
    static Mat4f create_rotation_x(float angle_rad);
    static Mat4f create_rotation_y(float angle_rad);
    static Mat4f create_rotation_z(float angle_rad);

private:
    std::array<float, 16> data;
};
