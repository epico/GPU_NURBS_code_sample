#ifndef GL_LOADER_H
#define GL_LOADER_H

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include <GL/glcorearb.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

inline bool init_gl_loader() {
    return true;
}

#endif // GL_LOADER_H
