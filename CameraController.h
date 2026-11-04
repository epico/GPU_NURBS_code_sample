#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include "gl_loader.h"
#include "math_eigen.h"

struct CameraController {
    vec3 position{0.0f, 0.0f, 2.0f};
    Quaternion rotation{Quaternion::Identity()};

    float azimuth = 0.0f;
    float angle = 0.0f;

    float strafe_speed = 4.0f;
    float forward_speed = 4.0f;
    float lift_speed = 4.0f;

    bool view_with_mouse = true;
    float key_view_speed_horizontal = 2.0f;
    float key_view_speed_vertical = 1.5f;

    float min_angle = -M_PI / 2.0f + 0.15f;
    float max_angle = M_PI / 2.0f - 0.15f;

    #define BASE_MOUSE_SENSITIVITY 1.22f
    float mouse_sensitivity = 0.0015f;

    CameraController();

    void lock_angle() {
        if (angle < min_angle) angle = min_angle;
        if (angle > max_angle) angle = max_angle;
    }

    void toggle_mouse_look();
    void on_mouse_move(double dx, double dy);
    void on_mouse_scroll(double yoffset, float dt);
    void update(GLFWwindow* window, float dt);

    mat4x4 get_view_matrix() const {
        return camera_view_matrix(position, rotation);
    }
};

#endif // CAMERA_CONTROLLER_H
