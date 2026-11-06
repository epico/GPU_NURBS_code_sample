#ifndef MATH_EIGEN_H
#define MATH_EIGEN_H

#include <cmath>
#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Geometry>

// Type aliases for graphics math
using vec3 = Eigen::Vector3f;
using vec4 = Eigen::Vector4f;
using mat4x4 = Eigen::Matrix4f;
using Quaternion = Eigen::Quaternionf;

// Create vec4 from vec3 and w
inline vec4 to_vec4(const vec3& v, float w = 1.0f) {
    return vec4(v.x(), v.y(), v.z(), w);
}

// Extract xyz from vec4
inline vec3 xyz(const vec4& v) {
    return v.head<3>();
}

// Create Quaternion from axis and angle (in radians)
inline Quaternion quaternion_from_axis_angle(const vec3& axis, float angle) {
    vec3 n = axis.normalized();
    return Quaternion(Eigen::AngleAxisf(angle, n));
}

// Compute OpenGL-compatible perspective projection matrix (column-major)
inline mat4x4 perspective_matrix(float fov_y_rad, float aspect, float zNear, float zFar) {
    float tanHalfFov = std::tan(fov_y_rad * 0.5f);
    mat4x4 m = mat4x4::Zero();
    m(0, 0) = 1.0f / (aspect * tanHalfFov);
    m(1, 1) = 1.0f / tanHalfFov;
    m(2, 2) = -(zFar + zNear) / (zFar - zNear);
    m(2, 3) = -(2.0f * zFar * zNear) / (zFar - zNear);
    m(3, 2) = -1.0f;
    return m;
}

inline Eigen::Matrix4f frustum_projection_matrix(float near_plane_distance,
                                                 float far_plane_distance,
                                                 float near_half_width,
                                                 float aspect_ratio) {
    float n = near_plane_distance;
    float f = far_plane_distance;
    float w = near_half_width;
    float h = aspect_ratio * near_half_width;

    Eigen::Matrix4f mat = Eigen::Matrix4f::Zero();

    // Standard OpenGL symmetric frustum matrix elements
    mat(0, 0) = n / w;
    mat(1, 1) = n / h;
    mat(2, 2) = -(f + n) / (f - n);
    mat(2, 3) = -(2.0f * f * n) / (f - n);
    mat(3, 2) = -1.0f; // Pushes z into w for perspective divide

    return mat;
}

// Compute camera view matrix V = R^T * T(-eye)
inline mat4x4 camera_view_matrix(const vec3& eye, const Quaternion& orientation) {
    Eigen::Matrix3f R = orientation.toRotationMatrix();
    mat4x4 V = mat4x4::Identity();
    V.block<3, 3>(0, 0) = R.transpose();
    V.block<3, 1>(0, 3) = -R.transpose() * eye;
    return V;
}

#endif // MATH_EIGEN_H
