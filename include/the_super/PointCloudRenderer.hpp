#pragma once

#include <filesystem>
#include <span>

#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "the_super/ShaderProgram.hpp"

namespace the_super {

struct PointSample {
    glm::vec3 position {};
    float intensity {1.0F};
    float group {};
};

class PointCloudRenderer {
public:
    explicit PointCloudRenderer(const std::filesystem::path& shaderDirectory);
    ~PointCloudRenderer();

    PointCloudRenderer(const PointCloudRenderer&) = delete;
    PointCloudRenderer& operator=(const PointCloudRenderer&) = delete;
    PointCloudRenderer(PointCloudRenderer&&) = delete;
    PointCloudRenderer& operator=(PointCloudRenderer&&) = delete;

    void upload(std::span<const PointSample> samples);
    void render(
        const glm::mat4& view,
        const glm::mat4& projection,
        float pointSize
    ) const;

    [[nodiscard]] GLsizei pointCount() const { return pointCount_; }

private:
    ShaderProgram shader_;
    GLuint vertexArray_ {};
    GLuint vertexBuffer_ {};
    GLsizei pointCount_ {};
    float intensityCeiling_ {1.0F};
};

} // namespace the_super
