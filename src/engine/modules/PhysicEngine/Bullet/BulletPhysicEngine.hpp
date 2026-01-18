/**
 * @file BulletPhysicEngine.hpp
 * @brief Bullet3-based physics simulation module
 * 
 * @details Provides physics simulation using Bullet Physics library.
 * Handles rigid body dynamics, collision detection, and physics queries.
 * 
 * @section channels_sub Subscribed Channels
 * | Channel | Payload | Description |
 * |---------|---------|-------------|
 * | `PhysicCommand` | Command string | Physics commands (see formats below) |
 * 
 * @section physic_commands PhysicCommand Formats
 * - `CreateBody:id,mass,friction,fixedRotation,useGravity;` - Create rigid body
 * - `DestroyBody:id;` - Remove rigid body
 * - `SetPosition:id:x,y,z;` - Set body position
 * - `SetRotation:id:rx,ry,rz;` - Set body rotation
 * - `SetVelocity:id:vx,vy,vz;` - Set linear velocity
 * - `ApplyForce:id,fx,fy,fz;` - Apply force to body
 * - `ApplyImpulse:id,ix,iy,iz;` - Apply impulse to body
 * - `SetAngularFactor:id:x,y,z;` - Constrain rotation axes
 * - `SetMass:id,mass;` - Update body mass
 * - `SetFriction:id,friction;` - Update friction coefficient
 * 
 * @section channels_pub Published Channels
 * | Channel | Payload | Description |
 * |---------|---------|-------------|
 * | `Collision` | "id1:id2" | Collision between two bodies |
 * | `PhysicUpdate` | Transform data | Body transform updates |
 * 
 * @see docs/CHANNELS.md for complete channel reference
 */

#pragma once

#include "../IPhysicEngine.hpp"
#include <btBulletDynamicsCommon.h>
#include <map>
#include <string>
#include <vector>
#include "BulletWorld.hpp"
#include "BulletBodyManager.hpp"

namespace rtypeEngine {

class BulletPhysicEngine : public IPhysicEngine {
  public:
    BulletPhysicEngine(const char* pubEndpoint, const char* subEndpoint);
    ~BulletPhysicEngine() override = default;

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
    void destroyBody(const std::string& id);

  private:
    void onPhysicCommand(const std::string& message);
    void stepSimulation();
    void sendUpdates();
    void checkCollisions();

    BulletWorld* _bulletWorld;
    BulletBodyManager* _bodyManager;

    std::chrono::high_resolution_clock::time_point _lastFrameTime;
    float _timeAccumulator = 0.0f;
    const float _maxDeltaTime = 1.0f / 30.0f;

};

} // namespace rtypeEngine
