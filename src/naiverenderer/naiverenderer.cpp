#include "naiverenderer.h"
#include "../atlodutil.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

NaiveRenderer::NaiveRenderer(Heightmap heightmap, float xzScale, float yScale)
//: _shader("../3d-terrain-with-lod/src/glsl/naiverenderer.vert", "../3d-terrain-with-lod/src/glsl/naiverenderer.frag")
{
    _shader = Shader("../3d-terrain-with-lod/src/glsl/naiverenderer.vert", "../3d-terrain-with-lod/src/glsl/naiverenderer.frag");
    _heightmap = heightmap;
    _xzScale = xzScale;
    _yScale = yScale;
    _height = heightmap.height();
    _width = heightmap.width();
    _hasTexture = false;
}

/**
 * @brief NaiveRenderer::~NaiveRenderer
 */
NaiveRenderer::~NaiveRenderer()
{
    std::cout << "Naive terrain destroyed" << std::endl;
}

/**
 * @brief NaiveRenderer::render
 */
void NaiveRenderer::render(Camera camera)
{
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RESTART_INDEX);

    if (_hasTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textureId);
        shader().setFloat("doTexture", 1.0f);
    } else {
        shader().setFloat("doTexture", 0.0f);
    }

    glBindVertexArray(_vao);

    glDrawElements(GL_TRIANGLE_STRIP, _nIndices * sizeof(unsigned int), GL_UNSIGNED_INT, (void*)0);

    GLenum error = glGetError();
    if (error != 0) {
        std::cout << "Error " << std::endl;
        std::exit(-1);
    }

    AtlodUtil::checkGlError("Naive algorithm render failed");
}

/**
 * @brief Initializes and loads the buffers for rendering
 *
 * This method should be called once before entering the main rendering loop.
 */
void NaiveRenderer::loadBuffers()
{
    loadNormals();

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    /* Set up index buffer */
    for (unsigned int i = 0; i < _height - 1; i++) {
        for (unsigned int j = 0; j < _width; j++) {
            _indices.push_back(j + _width * i);
            _indices.push_back(j + _width * (i + 1));
        }
        _indices.push_back(RESTART_INDEX);
    }

    _nIndices = _indices.size();

    glGenBuffers(1, &_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(unsigned int), &_indices[0], GL_STATIC_DRAW);

    /* Set up vertex buffer */
    int signedWidth = (int)_width;
    int signedHeight = (int)_height;
    for (unsigned int i = 0; i < _height; i++) {
        for (unsigned int j = 0; j < _width; j++) {

            float y = _heightmap.at(j, i);
            float x = (-signedWidth / 2.0f + signedWidth * j / (float)signedWidth);
            float z = (-signedHeight / 2.0f + signedHeight * i / (float)signedHeight);

            glm::vec3 normal = _normals[i * _width + j];

            /* Load vertices around center point */
            _vertices.push_back(x); /* position x */
            _vertices.push_back(y); /* position y */
            _vertices.push_back(z); /* position z */
            _vertices.push_back(normal.x); /* normal x */
            _vertices.push_back(normal.y); /* normal y */
            _vertices.push_back(normal.z); /* normal z */
            _vertices.push_back((float)j / (float)signedWidth); /* texture x */
            _vertices.push_back((float)i / (float)signedHeight); /* texture y */
        }
    }

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), &_vertices[0], GL_STATIC_DRAW);

    /* Position attribute */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    /* Normal attribute */
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    /* Texture attribute */
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    /* Vertices, indces and normals were loaded to the GPU, clear them */
    _indices.clear();
    _vertices.clear();
    _normals.clear();

    AtlodUtil::checkGlError("Naive algorithm load failed");
}

void NaiveRenderer::calculateNormal(unsigned j, unsigned int i, bool isBottomRight)
{
    int signedHeight = (int)_height;
    int signedWidth = (int)_width;

    int offset = 1;
    if (isBottomRight) {
        j++;
        i++;
        offset = -1;
    }

    float y = _heightmap.at(j, i);
    float x = (-signedWidth / 2.0f + signedWidth * j / (float)signedWidth);
    float z = (-signedHeight / 2.0f + signedHeight * i / (float)signedHeight);

    glm::vec3 v0(x, y, z);

    float y1 = _heightmap.at(j + offset, i);
    float x1 = (-signedWidth / 2.0f + signedWidth * (j + offset) / (float)signedWidth);
    float z1 = z;

    float y2 = _heightmap.at(j, i + offset);
    float x2 = x;
    float z2 = (-signedHeight / 2.0f + signedHeight * (i + offset) / (float)signedHeight);

    glm::vec3 v1(x1, y1, z1);
    glm::vec3 v2(x2, y2, z2);

    glm::vec3 a = v1 - v0;
    glm::vec3 b = v2 - v0;

    glm::vec3 normal = glm::cross(b, a);

    _normals[i * _width + j] += normal;
    _normals[i * _width + j + offset] += normal;
    _normals[(i + offset) * _width + j] += normal;
}

/**
 * @brief NaiveRenderer::loadNormals
 *
 * This normal calculating method is based on SLProject's calcNormals() method.
 * SLProject is developed at the Bern University of Applied Sciences.
 */
void NaiveRenderer::loadNormals()
{
    _normals.resize(_height * _width);

    for (unsigned int i = 0; i < _height - 1; i++) {
        for (unsigned int j = 0; j < _width - 1; j++) {
            _normals.push_back(glm::vec3(0.0f));
        }
    }

    /* Load normals for every vertex */
    for (unsigned int i = 0; i < _height - 1; i++) {
        for (unsigned int j = 0; j < _width - 1; j++) {
            calculateNormal(j, i, false);
            calculateNormal(j, i, true);
        }
    }

    /* Normalize each summed up corner vector */
    for (int i = 0; i < _normals.size(); i++) {
        _normals[i] = glm::normalize(_normals[i]);
    }
}

/**
 * @brief NaiveRenderer::unloadBuffers
 */
void NaiveRenderer::unloadBuffers()
{
    std::cout << "Unloading buffers" << std::endl;
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
    AtlodUtil::checkGlError("Naive algorithm unload failed");
}
