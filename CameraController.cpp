/*
 * CameraController behaviour.
 * Controls fly-around camera movement and mouse viewing.
 */
#include "CameraController.h"
#include <cmath>

CameraController::CameraController()
{
    float speed = 4.0f;
    strafe_speed = speed;
    forward_speed = speed;
    lift_speed = speed;
    key_view_speed_horizontal = 2.0f;
    key_view_speed_vertical = 1.5f;
    azimuth = 0.0f;
    angle = 0.0f;
    min_angle = -M_PI / 2.0f + 0.15f;
    max_angle = M_PI / 2.0f - 0.15f;
    view_with_mouse = true;
    mouse_sensitivity = 0.0015f;
    position = vec3(0.0f, 0.0f, 2.0f);
    rotation = Quaternion::Identity();
}

void CameraController::toggle_mouse_look()
{
    view_with_mouse = !view_with_mouse;
}

void CameraController::on_mouse_move(double dx, double dy)
{
    if (view_with_mouse) {
        azimuth -= BASE_MOUSE_SENSITIVITY * mouse_sensitivity * static_cast<float>(dx);
        angle += BASE_MOUSE_SENSITIVITY * mouse_sensitivity * static_cast<float>(dy);
        lock_angle();
    }
}

void CameraController::on_mouse_scroll(double yoffset, float dt)
{
    float factor = 1.0f + (dt * static_cast<float>(yoffset) * 5.0f);
    if (factor > 0.01f) {
        strafe_speed *= factor;
        forward_speed *= factor;
        lift_speed *= factor;
    }
}

void CameraController::update(GLFWwindow* window, float dt)
{
    float forward_movement = 0.0f;
    float side_movement = 0.0f;
    float lift = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) forward_movement += forward_speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) forward_movement -= forward_speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) side_movement -= strafe_speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) side_movement += strafe_speed;

    if (!view_with_mouse) {
        if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) azimuth += key_view_speed_horizontal * dt;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) azimuth -= key_view_speed_horizontal * dt;
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) angle -= key_view_speed_vertical * dt;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) angle += key_view_speed_vertical * dt;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) lift += lift_speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) lift -= lift_speed;

    lock_angle();
    float cos_azimuth = std::cos(azimuth);
    float sin_azimuth = std::sin(azimuth);
    vec3 forward(-sin_azimuth, 0.0f, -cos_azimuth);
    vec3 side(cos_azimuth, 0.0f, -sin_azimuth);

    position += dt * (side_movement * side + forward_movement * forward);
    position.y() += dt * lift;

    Quaternion q1 = quaternion_from_axis_angle(vec3(0, 1, 0), azimuth);
    Quaternion q2 = quaternion_from_axis_angle(side, angle);
    rotation = q2 * q1;
}
