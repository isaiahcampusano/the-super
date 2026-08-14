#include "the_super/PointCloudRenderer.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace the_super {

namespace {

struct GpuPoint {
    float x {};
    float y {};
    float z {};
    float intensity {};
    float group {};
};

} // namespace

PointCloudRenderer::PointCloudRenderer(const std::filesystem::path& shaderDirectory)
    : shader_(shaderDirectory / "cloud.vert", shaderDirectory / "cloud.frag") {
    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(GpuPoint)),
        reinterpret_cast<void*>(offsetof(GpuPoint, x))
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        1,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(GpuPoint)),
        reinterpret_cast<void*>(offsetof(GpuPoint, intensity))
    );
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        1,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(GpuPoint)),
        reinterpret_cast<void*>(offsetof(GpuPoint, group))
    );
    glBindVertexArray(0);
}

PointCloudRenderer::~PointCloudRenderer() {
    glDeleteBuffers(1, &vertexBuffer_);
    glDeleteVertexArrays(1, &vertexArray_);
}

void PointCloudRenderer::upload(std::span<const PointSample> samples) {
    std::vector<GpuPoint> points;
    points.reserve(samples.size());
    intensityCeiling_ = 1.0e-6F;
    for (const PointSample& sample : samples) {
        points.push_back({
            sample.position.x,
            sample.position.y,
            sample.position.z,
            sample.intensity,
            sample.group,
        });
        intensityCeiling_ = std::max(intensityCeiling_, sample.intensity);
    }

    pointCount_ = static_cast<GLsizei>(points.size());
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(points.size() * sizeof(GpuPoint)),
        points.data(),
        GL_STATIC_DRAW
    );
}

void PointCloudRenderer::render(
    const glm::mat4& view,
    const glm::mat4& projection,
    float pointSize
) const {
    if (pointCount_ == 0) {
        return;
    }
    shader_.use();
    shader_.set("view", view);
    shader_.set("projection", projection);
    shader_.set("pointSize", pointSize);
    shader_.set("intensityCeiling", intensityCeiling_);
    glDepthMask(GL_FALSE);
    glBindVertexArray(vertexArray_);
    glDrawArrays(GL_POINTS, 0, pointCount_);
    glDepthMask(GL_TRUE);
}

} // namespace the_super
