#pragma once

#include <btBulletDynamicsCommon.h>
#include <map>
#include <string>
#include <vector>

namespace rtypeEngine {
    class BulletBodyManager {
    public:
        BulletBodyManager(btDiscreteDynamicsWorld* world);
        ~BulletBodyManager();

        void createBody(const std::string& id, const std::string& type, const std::vector<float>& params);
        void destroyBody(const std::string& id);
        void clear();

        btRigidBody* getBody(const std::string& id);
        std::string getBodyId(btRigidBody* body);
        bool hasBody(const std::string& id) const;

        const std::map<std::string, btRigidBody*>& getBodies() const { return _bodies; }

        void applyForce(const std::string& id, const std::vector<float>& force);
        void setTransform(const std::string& id, const std::vector<float>& pos, const std::vector<float>& rot);
        void setLinearVelocity(const std::string& id, const std::vector<float>& velocity);
        void setAngularVelocity(const std::string& id, const std::vector<float>& velocity);
        void setMass(const std::string& id, float mass);
        void setFriction(const std::string& id, float friction);
        void setVelocityXZ(const std::string& id, float vx, float vz);
        void applyImpulse(const std::string& id, const std::vector<float>& impulse);
        void setAngularFactor(const std::string& id, const std::vector<float>& factor);

    private:
        btDiscreteDynamicsWorld* _dynamicsWorld; // Weak reference
        std::map<std::string, btRigidBody*> _bodies;
        std::map<btRigidBody*, std::string> _bodyIds;
    };
}
