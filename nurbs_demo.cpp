/*
 * Real-time quadratic Non-uniform rational B-splines on the GPU using OpenGL-4 and GLSL.
 * Standalone demo using GLFW and Eigen3.
 */

#include "gl_loader.h"
#include "math_eigen.h"
#include "shader.h"
#include "CameraController.h"

#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>

class DrawableNURBS {
public:
    DrawableNURBS(int _degree, int _m, int _n, const std::vector<vec3>& _control_grid) :
        degree{_degree},
        m{_m}, n{_n},
        num_vertical_knots{_m + _degree + 1},
        num_horizontal_knots{_n + _degree + 1}
    {
        assert(m > degree && n > degree);
        for (const auto& v : _control_grid) control_grid.push_back(v);
        for (int i = 0; i < num_vertical_knots; i++) {
            knots.push_back(static_cast<float>(i));
        }
        for (int i = 0; i < num_horizontal_knots; i++) {
            knots.push_back(static_cast<float>(i));
        }
        initialize_on_GPU();

        nurbs_program.add_shader(GLShader(VertexShader, "quadratic_NURBS.vert"));
        nurbs_program.add_shader(GLShader(TessControlShader, "quadratic_NURBS.tcs"));
        nurbs_program.add_shader(GLShader(TessEvaluationShader, "quadratic_NURBS.tes"));
        nurbs_program.add_shader(GLShader(GeometryShader, "quadratic_NURBS.geom"));
        nurbs_program.add_shader(GLShader(FragmentShader, "quadratic_NURBS.frag"));
        nurbs_program.link();
    }

    ~DrawableNURBS() {
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vertex_buffer) glDeleteBuffers(1, &vertex_buffer);
        if (index_buffer) glDeleteBuffers(1, &index_buffer);
        if (knot_texture_buffer) glDeleteBuffers(1, &knot_texture_buffer);
        if (knot_texture) glDeleteTextures(1, &knot_texture);
    }

    void update(float dt, float total_time) {
        // Move the surface around.
        for (size_t i = 0; i < control_grid.size(); i++) {
            control_grid[i].y() += 0.5f * dt *
                std::sin(3.0f * total_time + 10.0f * control_grid[i].x() * control_grid[i].z()) *
                std::cos(2.0f * total_time + control_grid[i].x() * control_grid[i].x());
        }
        // Perturb the knots to visualize the change in basis functions.
        for (int i = 0; i < num_vertical_knots; i++) {
            knots[i] += 0.4f * dt * std::sin(5.0f * total_time) * (2.0f * ((i + 1) % 2) - 1.0f);
        }
        for (int i = 0; i < num_horizontal_knots; i++) {
            knots[num_vertical_knots + i] += 0.4f * dt * std::cos(5.0f * total_time + i * 0.2f) * (2.0f * ((i + 1) % 2) - 1.0f);
        }

        sync_with_GPU();
    }

    void render(const mat4x4& view_proj, const mat4x4& model_matrix) {
        mat4x4 mvp_matrix = view_proj * model_matrix;

        // Render the NURBS surface wireframe
        nurbs_program.bind();
        glPatchParameteri(GL_PATCH_VERTICES, 9);
        glBindVertexArray(vao);
        glUniform1i(nurbs_program.uniform_location("num_vertical_knots"), num_vertical_knots);
        glUniform1i(nurbs_program.uniform_location("num_vertical_patches"), num_vertical_patches);
        glUniform1i(nurbs_program.uniform_location("num_horizontal_patches"), num_horizontal_patches);
        glUniform1i(nurbs_program.uniform_location("knots"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, knot_texture);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

        glUniformMatrix4fv(nurbs_program.uniform_location("mvp_matrix"), 1, GL_FALSE, mvp_matrix.data());

        glPointSize(10.0f);
        glLineWidth(1.0f);
        glDrawElements(GL_PATCHES, num_indices, index_type, (const void*)0);
        nurbs_program.unbind();
    }

private:
    void initialize_on_GPU() {
        int num_control_points = m * n;

        // Create index windows.
        int index_bytes;
        if (num_control_points - 1 <= 0xFF) {
            index_bytes = 1;
            index_type = GL_UNSIGNED_BYTE;
        } else if (num_control_points - 1 <= 0xFFFF) {
            index_bytes = 2;
            index_type = GL_UNSIGNED_SHORT;
        } else {
            index_bytes = 4;
            index_type = GL_UNSIGNED_INT;
        }
        num_vertical_patches = m - degree;
        num_horizontal_patches = n - degree;
        int num_patches = num_horizontal_patches * num_vertical_patches;
        num_indices = (degree + 1) * (degree + 1) * num_patches;

        // Create the index array.
        std::vector<uint8_t> index_array_bytes(index_bytes * num_indices);
        int pos = 0;
        for (uint32_t i = 0; i < static_cast<uint32_t>(num_vertical_patches); i++) {
            for (uint32_t j = 0; j < static_cast<uint32_t>(num_horizontal_patches); j++) {
                for (uint32_t ii = 0; ii <= static_cast<uint32_t>(degree); ii++) {
                    for (uint32_t jj = 0; jj <= static_cast<uint32_t>(degree); jj++) {
                        uint32_t index = (i + ii) * n + j + jj;
                        if (index_bytes == 1) {
                            index_array_bytes[pos] = static_cast<uint8_t>(index);
                        } else if (index_bytes == 2) {
                            reinterpret_cast<uint16_t*>(&index_array_bytes[0])[pos] = static_cast<uint16_t>(index);
                        } else {
                            reinterpret_cast<uint32_t*>(&index_array_bytes[0])[pos] = index;
                        }
                        pos++;
                    }
                }
            }
        }

        // Upload vertex data.
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * control_grid.size(), (const void*)&control_grid[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Upload index buffer.
        glGenBuffers(1, &index_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_array_bytes.size(), (const void*)&index_array_bytes[0], GL_STATIC_DRAW);
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Upload 1D texture of knots.
        glGenBuffers(1, &knot_texture_buffer);
        glBindBuffer(GL_TEXTURE_BUFFER, knot_texture_buffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(float) * knots.size(), (const void*)&knots[0], GL_STATIC_DRAW);
        glGenTextures(1, &knot_texture);
        glBindTexture(GL_TEXTURE_BUFFER, knot_texture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, knot_texture_buffer);
    }

    void sync_with_GPU() {
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * control_grid.size(), (const void*)&control_grid[0]);

        glBindBuffer(GL_TEXTURE_BUFFER, knot_texture_buffer);
        glBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(float) * knots.size(), (const void*)&knots[0]);
    }

    int degree;
    int m;
    int n;
    int num_vertical_knots;
    int num_horizontal_knots;
    int num_vertical_patches;
    int num_horizontal_patches;
    GLenum index_type;
    int num_indices;
    std::vector<vec3> control_grid;
    std::vector<float> knots;

    GLuint vao = 0;
    GLuint vertex_buffer = 0;
    GLuint knot_texture = 0;
    GLuint knot_texture_buffer = 0;
    GLuint index_buffer = 0;

    GLShaderProgram nurbs_program;
};

static bool first_mouse = true;
static double last_x = 0.0;
static double last_y = 0.0;

int main(int argc, char* argv[])
{
    printf("[main] Initializing GLFW...\n");
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    int window_width = 1280;
    int window_height = 720;
    printf("[main] Creating GLFW window (%dx%d)...\n", window_width, window_height);
    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "GPU NURBS Demo (Standalone / Eigen3)", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!init_gl_loader()) {
        std::cerr << "Failed to initialize OpenGL loader\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    CameraController camera;
    glfwSetWindowUserPointer(window, &camera);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double xpos, double ypos) {
        auto* cam = static_cast<CameraController*>(glfwGetWindowUserPointer(win));
        if (!cam) return;
        if (first_mouse) {
            last_x = xpos;
            last_y = ypos;
            first_mouse = false;
        }
        double dx = xpos - last_x;
        double dy = ypos - last_y;
        last_x = xpos;
        last_y = ypos;
        cam->on_mouse_move(dx, dy);
    });

    glfwSetScrollCallback(window, [](GLFWwindow* win, double /*xoffset*/, double yoffset) {
        auto* cam = static_cast<CameraController*>(glfwGetWindowUserPointer(win));
        if (cam) cam->on_mouse_scroll(yoffset, 0.016f);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* win, int key, int /*scancode*/, int action, int /*mods*/) {
        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
                glfwSetWindowShouldClose(win, GLFW_TRUE);
            }
            if (key == GLFW_KEY_E) {
                auto* cam = static_cast<CameraController*>(glfwGetWindowUserPointer(win));
                if (cam) {
                    cam->toggle_mouse_look();
                    if (cam->view_with_mouse) {
                        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                        first_mouse = true;
                    } else {
                        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    }
                }
            }
        }
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h) {
        glViewport(0, 0, w, h);
    });

    printf("[main] Initializing NURBS control net...\n");
    int m = 30;
    int n = 30;
    std::vector<vec3> positions;
    positions.reserve(m * n);
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            positions.push_back(vec3(0.2f * j, -2.0f, -0.2f * i));
        }
    }

    printf("[main] Creating DrawableNURBS...\n");
    DrawableNURBS nurbs(2, m, n, positions);

    glEnable(GL_DEPTH_TEST);

    printf("[main] Entering render loop...\n");
    double last_time = glfwGetTime();
    float total_time = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        double current_time = glfwGetTime();
        float dt = static_cast<float>(current_time - last_time);
        last_time = current_time;
        // Cap dt to avoid large jumps if window is moved/paused
        if (dt > 0.1f) dt = 0.1f;
        total_time += dt;

        camera.update(window, dt);
        nurbs.update(dt, total_time);

        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        glViewport(0, 0, fb_width, fb_height);

        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = fb_height > 0 ? static_cast<float>(fb_width) / static_cast<float>(fb_height) : 1.0f;
        mat4x4 proj = frustum_projection_matrix(0.1f, 300.0f, 0.1f, aspect);
        mat4x4 view = camera.get_view_matrix();
        mat4x4 view_proj = proj * view;
        mat4x4 model = mat4x4::Identity();

        nurbs.render(view_proj, model);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    printf("[main] Exiting...\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
