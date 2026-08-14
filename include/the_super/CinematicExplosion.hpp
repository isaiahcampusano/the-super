#pragma once

#include <filesystem>
#include <random>
#include <vector>

#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "the_super/ShaderProgram.hpp"

namespace the_super {

struct ExplosionParticle {
    glm::vec3 position {};
    glm::vec3 velocity {};
    glm::vec4 color {1.0F};
    float life {};
    float maxLife {};
    float size {};
};

class CinematicExplosion {
public:
    explicit CinematicExplosion(const std::filesystem::path& shaderDirectory);
    ~CinematicExplosion();

    CinematicExplosion(const CinematicExplosion&) = delete;
    CinematicExplosion& operator=(const CinematicExplosion&) = delete;

    void trigger(glm::vec3 center);
    void update(float deltaTime);
    void render(const glm::mat4& view, const glm::mat4& projection);
    void reset();

    [[nodiscard]] bool isActive() const { return active_; }
    [[nodiscard]] float elapsed() const { return elapsed_; }
    [[nodiscard]] float flashIntensity() const { return flashIntensity_; }
    [[nodiscard]] std::size_t particleCount() const { return particles_.size(); }

private:
    void spawnFireball();
    void spawnMushroomCloud();

    ShaderProgram particleShader_;
    ShaderProgram flashShader_;
    ShaderProgram shockwaveShader_;
    bool active_ {false};
    bool mushroomSpawned_ {false};
    float elapsed_ {};
    float flashIntensity_ {};
    glm::vec3 origin_ {0.0F};
    std::vector<ExplosionParticle> particles_;
    std::mt19937 generator_ {0xA70B0B5U};
    GLuint particleVertexArray_ {};
    GLuint particleVertexBuffer_ {};
    GLuint shockwaveVertexArray_ {};
    GLuint shockwaveVertexBuffer_ {};
    GLuint flashVertexArray_ {};
    GLsizei shockwaveVertexCount_ {};
};

} // namespace the_super
