#ifndef GPUTESSELLATION_H
#define GPUTESSELLATION_H

#include "../camera.h"
#include "../shader.h"
#include "../terrain.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../heightmap.h"

class GpuTessellation
{
public:
    GpuTessellation(Heightmap heightmap, float xzScale = 1.0f, float yScale = 1.0f);
    ~GpuTessellation();
    void loadBuffers();
    void render(Camera camera);
    void unloadBuffers();

private:
    void loadNormals();
    void calculateNormal(unsigned j, unsigned int i, bool isBottomRight);

    std::vector<float> _vertices;
    std::vector<unsigned int> _indices;
    std::vector<glm::vec3> _normals;

    unsigned int _vao, _vbo, _ebo;
    unsigned int _nIndices;
};

#endif // GPUTESSELLATION_H
