#pragma once

#include <vector>
#include <map>
#include <string>
#include <GL/glew.h>
#include "../I3DRenderer.hpp"

namespace rtypeEngine {

    struct Particle {
        Vector3f position;
        Vector3f velocity;
        Vector3f color;
        float life;
        float maxLife;
        float size;
    };

    struct ParticleGenerator {
        std::string id;
        Vector3f position;
        Vector3f rotation;
        Vector3f offset;
        Vector3f direction;
        float spread;
        float speed;
        float lifeTime;
        float rate;
        float size;
        Vector3f color;

        float accumulator = 0.0f;
        std::vector<Particle> particles;
    };

    class ParticleSystem {
    public:
        ParticleSystem();
        ~ParticleSystem() = default;

        void update(float dt);
        void render();

        void createGenerator(const std::string& id, const ParticleGenerator& gen);
        void updateGenerator(const std::string& id, const ParticleGenerator& gen);
        void destroyGenerator(const std::string& id);

        bool hasGenerator(const std::string& id) const;
        ParticleGenerator& getGenerator(const std::string& id);
        void setGeneratorPosition(const std::string& id, const Vector3f& pos);
        void setGeneratorRotation(const std::string& id, const Vector3f& rot);

    private:
        std::map<std::string, ParticleGenerator> _particleGenerators;
    };
}
