#include "shader.h"

/**
 * @brief Shader::Shader
 * @param vertexPath path to the vertex shader code
 * @param fragmentPath path to the fragment shader code
 */
Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    int success;
    char info[512];
    int vertexId = loadShaderProgram(vertexPath, GL_VERTEX_SHADER);
    int fragmentId = loadShaderProgram(fragmentPath, GL_FRAGMENT_SHADER);

    /* Create and link shader program */
    id = glCreateProgram();
    glAttachShader(id, vertexId);
    glAttachShader(id, fragmentId);
    glLinkProgram(id);

    /* Check linking errors */
    glGetProgramiv(id, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(id, 512, NULL, info);
        std::cout << "Shader linking failed: "
                  << info << std::endl;
    } else {
        std::cout << "Successfully linked shader" << std::endl;
    }

    /* Shaders are linked, therefore no longer necessary, delete them */
    glDeleteShader(vertexId);
    glDeleteShader(fragmentId);
}

/**
 * @brief Shader::loadShaderProgram
 * @param path
 * @param type
 * @return
 */
int Shader::loadShaderProgram(const char* path, GLenum type)
{
    std::string shaderCode;
    std::ifstream shaderFile;

    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        /* Open shader source file */
        shaderFile.open(path);

        std::stringstream shaderStream;

        /* Read file buffer contents into stream */
        shaderStream << shaderFile.rdbuf();

        /* Close file handler */
        shaderFile.close();

        /* Convert stream into string */
        shaderCode = shaderStream.str();

    } catch (std::ifstream::failure e) {
        std::cout << "Shader code could not be read." << std::endl;
    }

    const char* shaderCodeCStr = shaderCode.c_str();

    unsigned int shaderId;
    int success;
    char info[512];

    /* Create shader */
    shaderId = glCreateShader(type);
    glShaderSource(shaderId, 1, &shaderCodeCStr, NULL);
    glCompileShader(shaderId);

    /* Check shader compilation errors */
    if (!success) {
        glGetShaderInfoLog(shaderId, 512, NULL, info);
        std::cout << "Shader compilation failed: " << info << std::endl;
        std::cout << glGetError() << std::endl;
        exit(-1);
    };

    std::cout << "Compiled shader program with ID " << shaderId << std::endl;
    return shaderId;
}

/**
 * @brief Shader::handleErrors
 */
void Shader::handleErrors()
{
}

/**
 * @brief
 */
void Shader::use()
{
    glUseProgram(this->id);
}

/**
 * @brief Returns the shader program ID.
 * @return ID of the shader program
 */
unsigned int Shader::getId() const
{
    return id;
}

/**
 * @brief Shader::setBool
 * @param name
 * @param value
 */
void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
}

/**
 * @brief Shader::setInt
 * @param name
 * @param value
 */
void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(id, name.c_str()), value);
}

/**
 * @brief Shader::setFloat
 * @param name
 * @param value
 */
void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(id, name.c_str()), value);
}

/**
 * @brief Shader::setVec2
 * @param name
 * @param value
 */
void Shader::setVec2(const std::string& name, const glm::vec2& value) const
{
    glUniform2fv(glGetUniformLocation(id, name.c_str()), 1, &value[0]);
}

/**
 * @brief Shader::setVec2
 * @param name
 * @param x
 * @param y
 */
void Shader::setVec2(const std::string& name, float x, float y) const
{
    glUniform2f(glGetUniformLocation(id, name.c_str()), x, y);
}

/**
 * @brief Shader::setVec3
 * @param name
 * @param value
 */
void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(id, name.c_str()), 1, &value[0]);
}

/**
 * @brief Shader::setVec3
 * @param name
 * @param x
 * @param y
 * @param z
 */
void Shader::setVec3(const std::string& name, float x, float y, float z) const
{
    glUniform3f(glGetUniformLocation(id, name.c_str()), x, y, z);
}

/**
 * @brief Shader::setVec4
 * @param name
 * @param value
 */
void Shader::setVec4(const std::string& name, const glm::vec4& value) const
{
    glUniform4fv(glGetUniformLocation(id, name.c_str()), 1, &value[0]);
}

/**
 * @brief Shader::setVec4
 * @param name
 * @param x
 * @param y
 * @param z
 * @param w
 */
void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const
{
    glUniform4f(glGetUniformLocation(id, name.c_str()), x, y, z, w);
}

/**
 * @brief Shader::setMat2
 * @param name
 * @param mat
 */
void Shader::setMat2(const std::string& name, const glm::mat2& mat) const
{
    glUniformMatrix2fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

/**
 * @brief Shader::setMat3
 * @param name
 * @param mat
 */
void Shader::setMat3(const std::string& name, const glm::mat3& mat) const
{
    glUniformMatrix3fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

/**
 * @brief Shader::setMat4
 * @param name
 * @param mat
 */
void Shader::setMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
