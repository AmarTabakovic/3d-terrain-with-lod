#include "shader.h"

Shader::Shader() { }

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    int success;
    char info[512];
    int vertexId = loadShaderProgram(vertexPath, GL_VERTEX_SHADER);
    int fragmentId = loadShaderProgram(fragmentPath, GL_FRAGMENT_SHADER);

    /* Create and link shader program */
    _id = glCreateProgram();
    glAttachShader(_id, vertexId);
    glAttachShader(_id, fragmentId);
    glLinkProgram(_id);

    /* Check linking errors */
    glGetProgramiv(_id, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(_id, 512, NULL, info);
        std::cout << "Shader linking failed: "
                  << info << std::endl;
        std::exit(1);
    } else {
        std::cout << "Successfully linked shaders" << std::endl;
    }

    /* Shaders are linked, therefore no longer necessary, delete them */
    glDeleteShader(vertexId);
    glDeleteShader(fragmentId);
}

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
        std::exit(1);
    }

    const char* shaderCodeCStr = shaderCode.c_str();

    unsigned int shaderId;
    int success;
    char info[512];

    /* Create shader */
    shaderId = glCreateShader(type);
    glShaderSource(shaderId, 1, &shaderCodeCStr, NULL);
    glCompileShader(shaderId);
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);

    /* Check shader compilation errors */
    if (!success) {
        glGetShaderInfoLog(shaderId, 512, NULL, info);
        std::cout << "Shader compilation failed: " << info << std::endl;
        std::cout << glGetError() << std::endl;
        exit(1);
    };

    std::cout << "Compiled shader program with ID " << shaderId << std::endl;
    return shaderId;
}

void Shader::handleErrors()
{
}

void Shader::use()
{
    glUseProgram(this->_id);
}

unsigned int Shader::id() const
{
    return _id;
}

void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(_id, name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(_id, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(_id, name.c_str()), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const
{
    glUniform2fv(glGetUniformLocation(_id, name.c_str()), 1, &value[0]);
}

void Shader::setVec2(const std::string& name, float x, float y) const
{
    glUniform2f(glGetUniformLocation(_id, name.c_str()), x, y);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(_id, name.c_str()), 1, &value[0]);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const
{
    glUniform3f(glGetUniformLocation(_id, name.c_str()), x, y, z);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const
{
    glUniform4fv(glGetUniformLocation(_id, name.c_str()), 1, &value[0]);
}

void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const
{
    glUniform4f(glGetUniformLocation(_id, name.c_str()), x, y, z, w);
}

void Shader::setMat2(const std::string& name, const glm::mat2& mat) const
{
    glUniformMatrix2fv(glGetUniformLocation(_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat) const
{
    glUniformMatrix3fv(glGetUniformLocation(_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
