#include "ParticleSystem.hpp"
#include <random>
#include <cmath>
#include <iostream>

namespace rtypeEngine {

    ParticleSystem::ParticleSystem() {}

    void ParticleSystem::update(float dt)
    {
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        for (auto &pair : _particleGenerators)
        {
            auto &gen = pair.second;

            gen.accumulator += dt * gen.rate;
            while (gen.accumulator > 1.0f)
            {
                gen.accumulator -= 1.0f;

                Particle p;

                // Calculate world position and direction based on entity transform
                // Rotation order: Z, Y, X (matching Lua implementation)

                float radX = gen.rotation.x * 3.14159f / 180.0f;
                float radY = gen.rotation.y * 3.14159f / 180.0f;
                float radZ = gen.rotation.z * 3.14159f / 180.0f;

                float cx = cos(radX), sx = sin(radX);
                float cy = cos(radY), sy = sin(radY);
                float cz = cos(radZ), sz = sin(radZ);

                // Rotate offset
                float x = gen.offset.x;
                float y = gen.offset.y;
                float z = gen.offset.z;

                // Z rotation
                float x1 = x * cz - y * sz;
                float y1 = x * sz + y * cz;
                float z1 = z;

                // Y rotation
                float x2 = x1 * cy + z1 * sy;
                float y2 = y1;
                float z2 = -x1 * sy + z1 * cy;

                // X rotation
                float x3 = x2;
                float y3 = y2 * cx - z2 * sx;
                float z3 = y2 * sx + z2 * cx;

                p.position = {gen.position.x + x3, gen.position.y + y3, gen.position.z + z3};

                // Rotate direction
                Vector3f dir = gen.direction;

                // Z rotation
                float dx1 = dir.x * cz - dir.y * sz;
                float dy1 = dir.x * sz + dir.y * cz;
                float dz1 = dir.z;

                // Y rotation
                float dx2 = dx1 * cy + dz1 * sy;
                float dy2 = dy1;
                float dz2 = -dx1 * sy + dz1 * cy;

                // X rotation
                float dx3 = dx2;
                float dy3 = dy2 * cx - dz2 * sx;
                float dz3 = dy2 * sx + dz2 * cx;

                // Apply spread
                dx3 += dist(rng) * gen.spread;
                dy3 += dist(rng) * gen.spread;
                dz3 += dist(rng) * gen.spread;

                float len = sqrt(dx3 * dx3 + dy3 * dy3 + dz3 * dz3);
                if (len > 0)
                {
                    dx3 /= len;
                    dy3 /= len;
                    dz3 /= len;
                }

                p.velocity = {dx3 * gen.speed, dy3 * gen.speed, dz3 * gen.speed};
                p.life = gen.lifeTime;
                p.maxLife = gen.lifeTime;
                p.size = gen.size;
                p.color = gen.color;

                gen.particles.push_back(p);
            }

            for (auto it = gen.particles.begin(); it != gen.particles.end();)
            {
                it->life -= dt;
                if (it->life <= 0)
                {
                    it = gen.particles.erase(it);
                }
                else
                {
                    it->position.x += it->velocity.x * dt;
                    it->position.y += it->velocity.y * dt;
                    it->position.z += it->velocity.z * dt;
                    ++it;
                }
            }
        }
    }

    void ParticleSystem::render()
    {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        for (const auto &pair : _particleGenerators)
        {
            const auto &gen = pair.second;

            for (const auto &p : gen.particles)
            {
                float lifeRatio = (p.maxLife > 0.0f) ? p.life / p.maxLife : 0.0f;
                float alpha = lifeRatio;

                glColor4f(p.color.x, p.color.y, p.color.z, alpha);

                glPushMatrix();
                glTranslatef(p.position.x, p.position.y, p.position.z);

                float modelview[16];
                glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

                for (int i = 0; i < 3; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        if (i == j)
                            modelview[i * 4 + j] = 1.0f;
                        else
                            modelview[i * 4 + j] = 0.0f;
                    }
                }

                glLoadMatrixf(modelview);

                float s = p.size;
                glBegin(GL_QUADS);
                glTexCoord2f(0, 0);
                glVertex3f(-s, -s, 0);
                glTexCoord2f(1, 0);
                glVertex3f(s, -s, 0);
                glTexCoord2f(1, 1);
                glVertex3f(s, s, 0);
                glTexCoord2f(0, 1);
                glVertex3f(-s, s, 0);
                glEnd();

                glPopMatrix();
            }
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    void ParticleSystem::createGenerator(const std::string& id, const ParticleGenerator& gen) {
        _particleGenerators[id] = gen;
    }

    void ParticleSystem::updateGenerator(const std::string& id, const ParticleGenerator& gen) {
         if (_particleGenerators.find(id) != _particleGenerators.end()) {
             // Preserve existing state if needed, but for now we replace but keep particles?
             // The old code just replaced fields.
             // "auto& gen = _particleGenerators[id];" then update fields.
             // If we replace the whole struct we lose accumulator and particles.
             // So we should only update params.

             auto& existing = _particleGenerators[id];
             existing.position = gen.position;
             existing.direction = gen.direction;
             existing.spread = gen.spread;
             existing.speed = gen.speed;
             existing.lifeTime = gen.lifeTime;
             existing.rate = gen.rate;
             existing.size = gen.size;
             existing.color = gen.color;
             // Don't overwrite particles or accumulator
         }
    }

    void ParticleSystem::destroyGenerator(const std::string& id) {
        _particleGenerators.erase(id);
    }

    bool ParticleSystem::hasGenerator(const std::string& id) const {
        return _particleGenerators.find(id) != _particleGenerators.end();
    }

    ParticleGenerator& ParticleSystem::getGenerator(const std::string& id) {
        return _particleGenerators[id];
    }

    void ParticleSystem::setGeneratorPosition(const std::string& id, const Vector3f& pos) {
        if (hasGenerator(id)) {
            _particleGenerators[id].position = pos;
        }
    }

    void ParticleSystem::setGeneratorRotation(const std::string& id, const Vector3f& rot) {
        if (hasGenerator(id)) {
            _particleGenerators[id].rotation = rot;
        }
    }
}
