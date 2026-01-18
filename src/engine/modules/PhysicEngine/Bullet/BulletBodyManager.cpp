#include "BulletBodyManager.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace rtypeEngine {

    BulletBodyManager::BulletBodyManager(btDiscreteDynamicsWorld* world)
        : _dynamicsWorld(world) {}

    BulletBodyManager::~BulletBodyManager() {
        clear();
    }

    void BulletBodyManager::clear() {
        for (auto& pair : _bodies) {
            _dynamicsWorld->removeRigidBody(pair.second);
            delete pair.second->getMotionState();
            delete pair.second->getCollisionShape();
            delete pair.second;
        }
        _bodies.clear();
        _bodyIds.clear();
    }

    void BulletBodyManager::destroyBody(const std::string& id) {
        auto it = _bodies.find(id);
        if (it != _bodies.end()) {
            btRigidBody* body = it->second;
            _dynamicsWorld->removeRigidBody(body);
            delete body->getMotionState();
            delete body->getCollisionShape();
            delete body;
            _bodyIds.erase(body);
            _bodies.erase(it);
        }
    }

    btRigidBody* BulletBodyManager::getBody(const std::string& id) {
        auto it = _bodies.find(id);
        if (it != _bodies.end()) return it->second;
        return nullptr;
    }

    std::string BulletBodyManager::getBodyId(btRigidBody* body) {
        auto it = _bodyIds.find(body);
        if (it != _bodyIds.end()) return it->second;
        return "";
    }

    bool BulletBodyManager::hasBody(const std::string& id) const {
        return _bodies.find(id) != _bodies.end();
    }

    void BulletBodyManager::createBody(const std::string& id, const std::string& type, const std::vector<float>& params) {
        btCollisionShape* shape = nullptr;
        btScalar mass(1.f);
        btScalar friction(0.5f);

        std::string typeLower = type;
        std::transform(typeLower.begin(), typeLower.end(), typeLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (typeLower == "box" && params.size() >= 3) {
            shape = new btBoxShape(btVector3(params[0], params[1], params[2]));
            if (params.size() >= 4) mass = params[3];
            if (params.size() >= 5) friction = params[4];
        } else if (typeLower == "sphere" && params.size() >= 1) {
            shape = new btSphereShape(params[0]);
            if (params.size() >= 2) mass = params[1];
            if (params.size() >= 3) friction = params[2];
        } else {
            std::cerr << "[Bullet] ERROR: CreateBody unknown type or insufficient params for '" << id << "' (type='" << type << "', params=" << params.size() << ")" << std::endl;
            return;
        }

        if (!shape) {
            std::cerr << "[Bullet] ERROR: CreateBody failed to create shape for '" << id << "'" << std::endl;
            return;
        }

        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(0, 0, 0));

        bool isDynamic = (mass != 0.f);
        btVector3 localInertia(0, 0, 0);
        if (isDynamic) {
            shape->calculateLocalInertia(mass, localInertia);
        }

        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
        rbInfo.m_friction = friction;
        btRigidBody* body = new btRigidBody(rbInfo);

        _dynamicsWorld->addRigidBody(body);
        _bodies[id] = body;
        _bodyIds[body] = id;
        std::cout << "[Bullet] Created Body for ID: " << id << " (Mass: " << mass << ", Friction: " << friction << ")" << std::endl;
    }

    void BulletBodyManager::applyForce(const std::string& id, const std::vector<float>& force) {
        if (_bodies.find(id) != _bodies.end()) {
            _bodies[id]->activate(true);
            _bodies[id]->applyCentralForce(btVector3(force[0], force[1], force[2]));
        }
    }

    void BulletBodyManager::setTransform(const std::string& id, const std::vector<float>& pos, const std::vector<float>& rot) {
        if (_bodies.find(id) != _bodies.end()) {
            btRigidBody* body = _bodies[id];
            btTransform trans;
            trans.setIdentity();
            trans.setOrigin(btVector3(pos[0], pos[1], pos[2]));

            float degToRad = static_cast<float>(M_PI) / 180.0f;
            btQuaternion quat;
            quat.setEulerZYX(rot[1] * degToRad, rot[0] * degToRad, rot[2] * degToRad);
            trans.setRotation(quat);

            body->setWorldTransform(trans);
            if (body->getMotionState()) {
                body->getMotionState()->setWorldTransform(trans);
            }
            body->activate(true);
        }
    }

    void BulletBodyManager::setLinearVelocity(const std::string& id, const std::vector<float>& vel) {
        if (_bodies.find(id) == _bodies.end()) {
            std::cerr << "[Bullet] ERROR: SetLinearVelocity failed. Entity ID '" << id << "' does not exist in Physics World." << std::endl;
            return;
        }
        _bodies[id]->activate(true);
        _bodies[id]->setLinearVelocity(btVector3(vel[0], vel[1], vel[2]));
    }

    void BulletBodyManager::setAngularVelocity(const std::string& id, const std::vector<float>& vel) {
        if (_bodies.find(id) == _bodies.end()) {
            std::cerr << "[Bullet] ERROR: SetAngularVelocity failed. Entity ID '" << id << "' does not exist in Physics World." << std::endl;
            return;
        }
        _bodies[id]->activate(true);
        _bodies[id]->setAngularVelocity(btVector3(vel[0], vel[1], vel[2]));
    }

    void BulletBodyManager::setMass(const std::string& id, float mass) {
        if (_bodies.find(id) != _bodies.end()) {
            btRigidBody* body = _bodies[id];
            btVector3 localInertia(0, 0, 0);
            if (mass != 0.f) {
                body->getCollisionShape()->calculateLocalInertia(mass, localInertia);
            }
            body->setMassProps(mass, localInertia);
            body->updateInertiaTensor();
            body->activate(true);
        }
    }

    void BulletBodyManager::setFriction(const std::string& id, float friction) {
        if (_bodies.find(id) != _bodies.end()) {
            _bodies[id]->setFriction(friction);
            _bodies[id]->activate(true);
        }
    }

    void BulletBodyManager::setVelocityXZ(const std::string& id, float vx, float vz) {
        if (_bodies.find(id) != _bodies.end()) {
            btRigidBody* body = _bodies[id];
            body->activate(true);
            btVector3 vel = body->getLinearVelocity();
            vel.setX(vx);
            vel.setZ(vz);
            body->setLinearVelocity(vel);
        }
    }

    void BulletBodyManager::applyImpulse(const std::string& id, const std::vector<float>& impulse) {
        if (_bodies.find(id) != _bodies.end()) {
            _bodies[id]->activate(true);
            _bodies[id]->applyCentralImpulse(btVector3(impulse[0], impulse[1], impulse[2]));
        }
    }

    void BulletBodyManager::setAngularFactor(const std::string& id, const std::vector<float>& factor) {
        if (_bodies.find(id) != _bodies.end()) {
            _bodies[id]->activate(true);
            _bodies[id]->setAngularFactor(btVector3(factor[0], factor[1], factor[2]));
        }
    }

}
