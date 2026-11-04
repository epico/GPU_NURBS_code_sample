GPU_NURBS code sample
================================================================================
Real-time quadratic Non-uniform rational B-splines on the GPU using OpenGL-4 and GLSL.
The surface is visualized as a moving red wireframe with white control grid points,
viewable with a fly-around controllable camera. This application is standalone and
built using GLFW and Eigen3 (no cg_sandbox dependency).

"NURBS" are a hugely popular surface description used in computer-aided design.
The real-time rendering of these surfaces is an interesting problem.
How feasible is it for, e.g., video-games, to use CAD formats directly,
for high-quality non-polygonal surface rendering?

The main mathematical reference used in creating this demo is
    The NURBS book, Piegl and Tiller (1997).

![tessellation](https://github.com/LucasPayne/GPU_NURBS_code_sample/blob/main/images/tessellation.png?raw=true)

Building and Running
================================================================================
### Dependencies
- CMake (>= 3.16)
- C++17 compatible compiler (GCC or Clang)
- GLFW 3 (`glfw-devel`)
- Eigen3 (`eigen3-devel`)
- OpenGL 4.2+ development files (`mesa-libGL-devel`)

### Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Run
```bash
./build/nurbs_demo
```

Controls
================================================================================
- **W / S**: Move forward / backward
- **A / D**: Strafe left / right
- **Space / Left Shift**: Fly up / down
- **Mouse**: Look around (pitch and yaw)
- **E**: Toggle mouse capture / free cursor
- **Scroll Wheel**: Adjust movement speed
- **H / J / K / L**: Keyboard look controls (when mouse capture is off)
- **Q / Escape**: Quit demo

Overview of program structure
================================================================================
GLSL shaders
--------------------------------------------------------------------------------
- *quadratic_NURBS.{vert,tcs,tes,geom,frag}*:
Five shader stages (vertex, tessellation control, tessellation evaluation,
geometry, and fragment processing) are compiled and linked into one GPU program.
  - The vertex shader passes data along to the control shader.
  - The tessellation control shader sets constant tessellation parameters
   (determining how many u,v coordinates are generated).
  - The tessellation evaluation shader computes the corresponding point on
   the NURBS surface, then performs the view and projection transformations.
  - The geometry shader expands triangle primitives into three lines each,
   generating a wireframe.
  - The fragment shader outputs constant red.
  The result (alongside the CPU code) is a (non-antialiased) red wireframe
  quadratic NURBS surface with a variable number of patches and arbitrary
  knot vectors.

C++ source
--------------------------------------------------------------------------------
- *math_eigen.h*:
3D graphics math definitions (vectors, matrices, quaternions, perspective projection,
and camera view transforms) using Eigen3.
- *gl_loader.h*:
Lightweight OpenGL 4.2 core profile loader.
- *shader.h*:
Shader compilation and program linking utility with multi-stage pipeline support.
- *CameraController.{h,cpp}*:
Mouse view and fly-around movement controls.
- *nurbs_demo.cpp*:
Main application entry point and `DrawableNURBS` implementation managing the GPU
buffers, knot textures, dynamic surface wave animation, and rendering loop.
