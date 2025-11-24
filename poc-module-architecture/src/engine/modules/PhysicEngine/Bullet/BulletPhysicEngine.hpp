#pragma once

#include "../IPhysicEngine.hpp"
#include <btBulletDynamicsCommon.h>
#include <map>
#include <string>
#include <vector>

namespace rtypeEngine {

class BulletPhysicEngine : public IPhysicEngine {
  public:
    BulletPhysicEngine(const char* pubEndpoint, const char* subEndpoint);
    ~BulletPhysicEngine() override;

    void init() override;
    void loop() override;
    void cleanup() override;

  protected:
    void createBody(const std::string& id, const std::string& type, const std::vector<float>& params) override;
    void setTransform(const std::string& id, const std::vector<float>& pos, const std::vector<float>& rot) override;
    void applyForce(const std::string& id, const std::vector<float>& force) override;
    void raycast(const std::vector<float>& origin, const std::vector<float>& direction);
    void setLinearVelocity(const std::string& id, const std::vector<float>& velocity);
    void setAngularVelocity(const std::string& id, const std::vector<float>& velocity);
    void setMass(const std::string& id, float mass);
    void setFriction(const std::string& id, float friction);
    void setVelocityXZ(const std::string& id, float vx, float vz);
    void applyImpulse(const std::string& id, const std::vector<float>& impulse);
    void setAngularFactor(const std::string& id, const std::vector<float>& factor);

  private:
    void onPhysicCommand(const std::string& message);
    void stepSimulation();
    void sendUpdates();
    void checkCollisions();

    btDefaultCollisionConfiguration* _collisionConfiguration;
    btCollisionDispatcher* _dispatcher;
    btBroadphaseInterface* _overlappingPairCache;
    btSequentialImpulseConstraintSolver* _solver;
    btDiscreteDynamicsWorld* _dynamicsWorld;

    std::map<std::string, btRigidBody*> _bodies;
    std::map<btRigidBody*, std::string> _bodyIds;

    std::chrono::high_resolution_clock::time_point _lastFrameTime;
    float _timeAccumulator = 0.0f;
    const float _maxDeltaTime = 1.0f / 30.0f;

};

} // namespace rtypeEngine
