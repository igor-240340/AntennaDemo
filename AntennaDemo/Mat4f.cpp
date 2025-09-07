#include <cmath>
#include <algorithm>

#include "Mat4f.h"

// Identity.
Mat4f::Mat4f() {
    std::ranges::fill(data, 0.0f);

    data[0] = 1.0f;
    data[5] = 1.0f;
    data[10] = 1.0f;
    data[15] = 1.0f;
}

Vec4f Mat4f::operator*(const Vec4f& vec_right_column) const {
    return Vec4f{
        data[0] * vec_right_column.x + data[4] * vec_right_column.y + data[8] * vec_right_column.z + data[12] * vec_right_column.w,
        data[1] * vec_right_column.x + data[5] * vec_right_column.y + data[9] * vec_right_column.z + data[13] * vec_right_column.w,
        data[2] * vec_right_column.x + data[6] * vec_right_column.y + data[10] * vec_right_column.z + data[14] * vec_right_column.w,
        data[3] * vec_right_column.x + data[7] * vec_right_column.y + data[11] * vec_right_column.z + data[15] * vec_right_column.w
    };
}

// NOTE: Читабельность кода пока в приоритете, поэтому реализация наивная.
Mat4f Mat4f::operator*(const Mat4f& mat_right_columns) const {
    const Vec4f left_row_0{ data[0], data[4], data[8], data[12] };
    const Vec4f left_row_1{ data[1], data[5], data[9], data[13] };
    const Vec4f left_row_2{ data[2], data[6], data[10], data[14] };
    const Vec4f left_row_3{ data[3], data[7], data[11], data[15] };

    const std::array<float, 16>& right_data = mat_right_columns.data;
    const Vec4f right_col_0{ right_data[0], right_data[1], right_data[2], right_data[3] };
    const Vec4f right_col_1{ right_data[4], right_data[5], right_data[6], right_data[7] };
    const Vec4f right_col_2{ right_data[8], right_data[9], right_data[10], right_data[11] };
    const Vec4f right_col_3{ right_data[12], right_data[13], right_data[14], right_data[15] };

    Vec4f prod_row_0{
        Vec4f::dot(left_row_0, right_col_0),
        Vec4f::dot(left_row_0, right_col_1),
        Vec4f::dot(left_row_0, right_col_2),
        Vec4f::dot(left_row_0, right_col_3)
    };

    Vec4f prod_row_1{
        Vec4f::dot(left_row_1, right_col_0),
        Vec4f::dot(left_row_1, right_col_1),
        Vec4f::dot(left_row_1, right_col_2),
        Vec4f::dot(left_row_1, right_col_3)
    };

    Vec4f prod_row_2{
        Vec4f::dot(left_row_2, right_col_0),
        Vec4f::dot(left_row_2, right_col_1),
        Vec4f::dot(left_row_2, right_col_2),
        Vec4f::dot(left_row_2, right_col_3)
    };

    Vec4f prod_row_3{
        Vec4f::dot(left_row_3, right_col_0),
        Vec4f::dot(left_row_3, right_col_1),
        Vec4f::dot(left_row_3, right_col_2),
        Vec4f::dot(left_row_3, right_col_3)
    };

    Mat4f prod;
    prod.data[0] = prod_row_0.x;
    prod.data[4] = prod_row_0.y;
    prod.data[8] = prod_row_0.z;
    prod.data[12] = prod_row_0.w;

    prod.data[1] = prod_row_1.x;
    prod.data[5] = prod_row_1.y;
    prod.data[9] = prod_row_1.z;
    prod.data[13] = prod_row_1.w;

    prod.data[2] = prod_row_2.x;
    prod.data[6] = prod_row_2.y;
    prod.data[10] = prod_row_2.z;
    prod.data[14] = prod_row_2.w;

    prod.data[3] = prod_row_3.x;
    prod.data[7] = prod_row_3.y;
    prod.data[11] = prod_row_3.z;
    prod.data[15] = prod_row_3.w;
    return prod;
}

Mat4f Mat4f::create_identity() {
    return Mat4f{};
}

// NOTE: Вывод см. в /docs/persp_proj_mat_2/persp_proj.mcdx.
Mat4f Mat4f::create_perspective(float fov_vert_rad, float aspect_ratio, float near, float far) {
    Mat4f mat_proj{};

    mat_proj.data[0] = 1.0f / (aspect_ratio * std::tanf(fov_vert_rad / 2.0f));
    mat_proj.data[5] = 1.0f / std::tanf(fov_vert_rad / 2.0f);
    mat_proj.data[10] = -far / (far - near);
    mat_proj.data[11] = -1.0f;
    mat_proj.data[14] = -(far * near) / (far - near);
    mat_proj.data[15] = 0.0f;

    return mat_proj;
}

// NOTE: Вывод см. в /docs/ortho_proj/ortho_proj.mcdx.
Mat4f Mat4f::create_ortho(float left, float right, float bottom, float top, float near, float far) {
    Mat4f mat_proj{};

    mat_proj.data[0] = 2.0f / (right - left);
    mat_proj.data[5] = 2.0f / (top - bottom);
    mat_proj.data[10] = -1.0f / (far - near);
    mat_proj.data[12] = -(right + left) / (right - left);
    mat_proj.data[13] = -(top + bottom) / (top - bottom);
    mat_proj.data[14] = -near / (far - near);

    return mat_proj;
}

// NOTE: Вывод см. в /docs/persp_proj_mat_2/persp_proj.mcdx.
Mat4f Mat4f::create_viewport(int w, int h) {
    Mat4f mat_view{};

    mat_view.data[0] = (w - 1) / 2.0f;
    mat_view.data[5] = -(h - 1) / 2.0f;
    mat_view.data[12] = (w - 1) / 2.0f;
    mat_view.data[13] = (h - 1) / 2.0f;

    return mat_view;
}

Mat4f Mat4f::create_look_at(const Vec3f& pos, const Vec3f& look_dir, const Vec3f& up) {
    Vec3f right = Vec3f::cross(up, look_dir);

    Mat4f view_mat{};
    view_mat.data.at(0) = right.x;
    view_mat.data.at(4) = right.y;
    view_mat.data.at(8) = right.z;
    view_mat.data.at(12) = -Vec3f::dot(right, pos);

    view_mat.data.at(1) = up.x;
    view_mat.data.at(5) = up.y;
    view_mat.data.at(9) = up.z;
    view_mat.data.at(13) = -Vec3f::dot(up, pos);

    view_mat.data.at(2) = look_dir.x;
    view_mat.data.at(6) = look_dir.y;
    view_mat.data.at(10) = look_dir.z;
    view_mat.data.at(14) = -Vec3f::dot(look_dir, pos);

    return view_mat;
}

// NOTE: Теорию и пруфы для всех матриц трансформации см. в /docs/transformations.
Mat4f Mat4f::create_translation(const Vec3f& v) {
    Mat4f mat_trans{};

    mat_trans.data[12] = v.x;
    mat_trans.data[13] = v.y;
    mat_trans.data[14] = v.z;

    return mat_trans;
}

Mat4f Mat4f::create_rotation_x(const float angle_rad) {
    Mat4f mat_rot{};

    const float cos = std::cos(angle_rad);
    const float sin = std::sin(angle_rad);

    mat_rot.data[5] = cos;
    mat_rot.data[6] = sin;
    mat_rot.data[9] = -sin;
    mat_rot.data[10] = cos;

    return mat_rot;
}

Mat4f Mat4f::create_rotation_y(const float angle_rad) {
    Mat4f mat_rot{};

    const float cos = std::cos(angle_rad);
    const float sin = std::sin(angle_rad);

    mat_rot.data[0] = cos;
    mat_rot.data[2] = -sin;
    mat_rot.data[8] = sin;
    mat_rot.data[10] = cos;

    return mat_rot;
}

Mat4f Mat4f::create_rotation_z(const float angle_rad) {
    Mat4f mat_rot{};

    const float cos = std::cos(angle_rad);
    const float sin = std::sin(angle_rad);

    mat_rot.data[0] = cos;
    mat_rot.data[1] = sin;
    mat_rot.data[4] = -sin;
    mat_rot.data[5] = cos;

    return mat_rot;
}
