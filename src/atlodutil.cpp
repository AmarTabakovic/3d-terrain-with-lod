#include "atlodutil.h"

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

void AtlodUtil::checkGlError(const std::string& message)
{
    GLenum error = glGetError();
    if (error != 0) {
        std::cerr << "Error: " << message << std::endl;
        std::cerr << "OpenGL error code: " << error << std::endl;
        std::exit(1);
    }
}
