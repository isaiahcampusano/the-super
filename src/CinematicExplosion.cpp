#include "the_super/CinematicExplosion.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

namespace the_super {

namespace {

constexpr float effectDuration = 4.0F;
constexpr std::size_t fireballParticleCount = 500;
constexpr std::size_t mushroomParticleCount = 1'000;
constexpr int shockwaveSegments = 160;

glm::vec3 randomDirection(std::mt19937& generator) {
    std::uniform_real_distribution<float> component(-1.0F, 1.0F);
    glm::vec3 direction;
    do {
        direction = {component(generator), component(generator), component(generator)};
    } while (glm::dot(direction, direction) < 1.0e-4F
        || glm::dot(direction, direction) > 1.0F);
    return glm::normalize(direction);
}

float randomRange(std::mt19937& generator, float minimum, float maximum) {
    return std::uniform_real_distribution<float>(minimum, maximum)(generator);
}

} // namespace

CinematicExplosion::CinematicExplosion(const std::filesystem::path& shaderDirectory)
    : particleShader_(shaderDirectory / "explosion.vert", shaderDirectory / "explosion.frag"),
      flashShader_(shaderDirectory / "flash.vert", shaderDirectory / "flash.frag"),
      shockwaveShader_(shaderDirectory / "shockwave.vert", shaderDirectory / "shockwave.frag") {
    glGenVertexArrays(1, &particleVertexArray_);
    glGenBuffers(1, &particleVertexBuffer_);
    glBindVertexArray(particleVertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, particleVertexBuffer_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(ExplosionParticle)),
        reinterpret_cast<void*>(offsetof(ExplosionParticle, position))
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(ExplosionParticle)),
        reinterpret_cast<void*>(offsetof(ExplosionParticle, color))
    );
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        1,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(ExplosionParticle)),
        reinterpret_cast<void*>(offsetof(ExplosionParticle, size))
    );

    std::array<glm::vec3, shockwaveSegments> ringVertices {};
    for (int segment = 0; segment < shockwaveSegments; ++segment) {
        const float angle = static_cast<float>(segment)
            / static_cast<float>(shockwaveSegments)
            * 2.0F
            * std::numbers::pi_v<float>;
        ringVertices[static_cast<std::size_t>(segment)] = {
            std::cos(angle),
            0.0F,
            std::sin(angle),
        };
    }
    shockwaveVertexCount_ = static_cast<GLsizei>(ringVertices.size());
    glGenVertexArrays(1, &shockwaveVertexArray_);
    glGenBuffers(1, &shockwaveVertexBuffer_);
    glBindVertexArray(shockwaveVertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, shockwaveVertexBuffer_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(sizeof(ringVertices)),
        ringVertices.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glGenVertexArrays(1, &flashVertexArray_);
    glBindVertexArray(0);
}

CinematicExplosion::~CinematicExplosion() {
    glDeleteVertexArrays(1, &flashVertexArray_);
    glDeleteBuffers(1, &shockwaveVertexBuffer_);
    glDeleteVertexArrays(1, &shockwaveVertexArray_);
    glDeleteBuffers(1, &particleVertexBuffer_);
    glDeleteVertexArrays(1, &particleVertexArray_);
}

void CinematicExplosion::trigger(glm::vec3 center) {
    reset();
    active_ = true;
    origin_ = center;
    generator_.seed(0xA70B0B5U);
    particles_.reserve(fireballParticleCount + mushroomParticleCount);
    spawnFireball();
}

void CinematicExplosion::reset() {
    active_ = false;
    mushroomSpawned_ = false;
    elapsed_ = 0.0F;
    flashIntensity_ = 0.0F;
    particles_.clear();
}

void CinematicExplosion::spawnFireball() {
    for (std::size_t index = 0; index < fireballParticleCount; ++index) {
        const glm::vec3 direction = randomDirection(generator_);
        const float life = randomRange(generator_, 1.45F, 2.15F);
        particles_.push_back({
            origin_ + direction * randomRange(generator_, 0.0F, 0.32F),
            direction * randomRange(generator_, 1.8F, 4.2F),
            glm::vec4 {1.0F, 0.95F, 0.65F, 1.0F},
            life,
            life,
            randomRange(generator_, 16.0F, 34.0F),
        });
    }
}

void CinematicExplosion::spawnMushroomCloud() {
    for (std::size_t index = 0; index < mushroomParticleCount; ++index) {
        const bool isStem = index < mushroomParticleCount / 3U;
        const float angle = randomRange(generator_, 0.0F, 2.0F * std::numbers::pi_v<float>);
        const float radius = isStem
            ? randomRange(generator_, 0.0F, 0.38F)
            : randomRange(generator_, 0.25F, 1.15F);
        const glm::vec3 horizontal {std::cos(angle), std::sin(angle), 0.0F};
        const float life = randomRange(generator_, 2.7F, 3.6F);
        const float upwardSpeed = isStem
            ? randomRange(generator_, 1.8F, 3.1F)
            : randomRange(generator_, 1.15F, 2.2F);
        const float outwardSpeed = isStem
            ? randomRange(generator_, 0.02F, 0.22F)
            : randomRange(generator_, 0.55F, 1.55F);
        particles_.push_back({
            origin_ + horizontal * radius + glm::vec3 {0.0F, 0.0F, isStem ? -0.35F : 0.35F},
            horizontal * outwardSpeed + glm::vec3 {0.0F, 0.0F, upwardSpeed},
            glm::vec4 {1.0F, 0.42F, 0.06F, 0.9F},
            life,
            life,
            randomRange(generator_, 13.0F, 30.0F),
        });
    }
    mushroomSpawned_ = true;
}

void CinematicExplosion::update(float deltaTime) {
    if (!active_) {
        return;
    }
    const float step = std::clamp(deltaTime, 0.0F, 0.05F);
    elapsed_ += step;

    if (elapsed_ < 0.06F) {
        flashIntensity_ = elapsed_ / 0.06F;
    } else if (elapsed_ < 0.30F) {
        flashIntensity_ = std::exp(-12.0F * (elapsed_ - 0.06F));
    } else {
        flashIntensity_ = 0.0F;
    }
    if (!mushroomSpawned_ && elapsed_ >= 0.50F) {
        spawnMushroomCloud();
    }

    for (ExplosionParticle& particle : particles_) {
        if (particle.life <= 0.0F) {
            continue;
        }
        particle.life -= step;
        particle.position += particle.velocity * step;
        const float age = 1.0F - std::max(particle.life, 0.0F) / particle.maxLife;

        if (particle.maxLife < 2.5F) {
            particle.velocity *= std::pow(0.985F, step * 60.0F);
            const glm::vec3 white {1.0F, 1.0F, 0.92F};
            const glm::vec3 yellow {1.0F, 0.72F, 0.05F};
            const glm::vec3 orange {1.0F, 0.20F, 0.01F};
            const glm::vec3 ash {0.18F, 0.16F, 0.16F};
            const glm::vec3 color = age < 0.22F
                ? glm::mix(white, yellow, age / 0.22F)
                : age < 0.68F
                    ? glm::mix(yellow, orange, (age - 0.22F) / 0.46F)
                    : glm::mix(orange, ash, (age - 0.68F) / 0.32F);
            particle.color = glm::vec4 {color, 1.0F - age};
            particle.size *= std::pow(1.006F, step * 60.0F);
        } else {
            particle.velocity.z += 0.24F * step;
            particle.velocity.x *= std::pow(0.993F, step * 60.0F);
            particle.velocity.y *= std::pow(0.993F, step * 60.0F);
            const glm::vec3 ember {0.95F, 0.24F, 0.03F};
            const glm::vec3 smoke {0.16F, 0.15F, 0.17F};
            particle.color = glm::vec4 {glm::mix(ember, smoke, std::min(age * 1.4F, 1.0F)), 0.82F * (1.0F - age)};
            particle.size *= std::pow(1.003F, step * 60.0F);
        }
    }

    std::erase_if(particles_, [](const ExplosionParticle& particle) {
        return particle.life <= 0.0F;
    });
    if (elapsed_ >= effectDuration) {
        reset();
    }
}

void CinematicExplosion::render(const glm::mat4& view, const glm::mat4& projection) {
    if (!active_) {
        return;
    }

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    if (!particles_.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, particleVertexBuffer_);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(particles_.size() * sizeof(ExplosionParticle)),
            particles_.data(),
            GL_STREAM_DRAW
        );
        particleShader_.use();
        particleShader_.set("view", view);
        particleShader_.set("projection", projection);
        glBindVertexArray(particleVertexArray_);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particles_.size()));
    }

    if (elapsed_ >= 0.20F && elapsed_ <= 1.50F) {
        const float progress = (elapsed_ - 0.20F) / 1.30F;
        const float radius = 0.8F + progress * 7.0F;
        glm::mat4 model = glm::translate(glm::mat4 {1.0F}, origin_);
        model = glm::scale(model, glm::vec3 {radius});
        shockwaveShader_.use();
        shockwaveShader_.set("model", model);
        shockwaveShader_.set("view", view);
        shockwaveShader_.set("projection", projection);
        shockwaveShader_.set("color", glm::vec3 {1.0F, 0.62F, 0.16F});
        shockwaveShader_.set("alpha", (1.0F - progress) * 0.85F);
        glLineWidth(4.0F);
        glBindVertexArray(shockwaveVertexArray_);
        glDrawArrays(GL_LINE_LOOP, 0, shockwaveVertexCount_);
        glLineWidth(1.0F);
    }

    if (flashIntensity_ > 0.001F) {
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        flashShader_.use();
        flashShader_.set("intensity", flashIntensity_);
        glBindVertexArray(flashVertexArray_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glEnable(GL_DEPTH_TEST);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

} // namespace the_super
