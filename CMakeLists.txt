cmake_minimum_required(VERSION 3.16)
project(GPU_NURBS_demo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Eigen3 REQUIRED NO_MODULE)
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

add_executable(nurbs_demo
    nurbs_demo.cpp
    CameraController.cpp
)

target_include_directories(nurbs_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(nurbs_demo PRIVATE
    Eigen3::Eigen
    glfw
    OpenGL::GL
    m
    ${CMAKE_DL_LIBS}
)
