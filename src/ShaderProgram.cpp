#include "the_super/ShaderProgram.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <glm/gtc/type_ptr.hpp>

namespace the_super {

namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream stream(path);
    if (!stream) {
        throw std::runtime_error("Could not open shader: " + path.string());
    }

    std::ostringstream contents;
    contents << stream.rdbuf();
    return contents.str();
}

GLuint compileShader(GLenum type, const std::string& source, const char* label) {
    const GLuint shader = glCreateShader(type);
    const char* sourcePointer = source.c_str();
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<std::size_t>(logLength), '\0');
    glGetShaderInfoLog(shader, logLength, nullptr, log.data());
    glDeleteShader(shader);
    throw std::runtime_error(std::string("Failed to compile ") + label + ":\n" + log);
}

} // namespace

ShaderProgram::ShaderProgram(
    const std::filesystem::path& vertexPath,
    const std::filesystem::path& fragmentPath
) {
    const std::string vertexSource = readFile(vertexPath);
    const std::string fragmentSource = readFile(fragmentPath);
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, "vertex shader");

    GLuint fragmentShader = 0;
    try {
        fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, "fragment shader");
    } catch (...) {
        glDeleteShader(vertexShader);
        throw;
    }

    id_ = glCreateProgram();
    glAttachShader(id_, vertexShader);
    glAttachShader(id_, fragmentShader);
    glLinkProgram(id_);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(id_, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return;
    }

    GLint logLength = 0;
    glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<std::size_t>(logLength), '\0');
    glGetProgramInfoLog(id_, logLength, nullptr, log.data());
    glDeleteProgram(id_);
    id_ = 0;
    throw std::runtime_error("Failed to link shader program:\n" + log);
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(id_);
}

void ShaderProgram::use() const {
    glUseProgram(id_);
}

GLint ShaderProgram::uniformLocation(std::string_view name) const {
    const std::string nullTerminatedName(name);
    return glGetUniformLocation(id_, nullTerminatedName.c_str());
}

void ShaderProgram::set(std::string_view name, const glm::mat4& value) const {
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::set(std::string_view name, const glm::vec3& value) const {
    glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void ShaderProgram::set(std::string_view name, float value) const {
    glUniform1f(uniformLocation(name), value);
}

void ShaderProgram::set(std::string_view name, int value) const {
    glUniform1i(uniformLocation(name), value);
}

} // namespace the_super
