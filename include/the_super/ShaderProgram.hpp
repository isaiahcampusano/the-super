#pragma once

#include <filesystem>
#include <string_view>

#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace the_super {

class ShaderProgram {
public:
    ShaderProgram(
        const std::filesystem::path& vertexPath,
        const std::filesystem::path& fragmentPath
    );
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&&) = delete;
    ShaderProgram& operator=(ShaderProgram&&) = delete;

    void use() const;
    void set(std::string_view name, const glm::mat4& value) const;
    void set(std::string_view name, const glm::vec3& value) const;
    void set(std::string_view name, float value) const;
    void set(std::string_view name, int value) const;

private:
    [[nodiscard]] GLint uniformLocation(std::string_view name) const;

    GLuint id_ {};
};

} // namespace the_super
