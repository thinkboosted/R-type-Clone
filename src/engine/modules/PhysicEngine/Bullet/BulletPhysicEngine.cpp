#include "BulletPhysicEngine.hpp"
#include <iostream>
#include <sstream>
#include <vector>

static void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

static float safeStof(const std::string &str, float fallback = 0.0f) {
    try {
        return std::stof(str);
    } catch (const std::exception &) {
        return fallback;
    }
}
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace rtypeEngine {

BulletPhysicEngine::BulletPhysicEngine(const char* pubEndpoint, const char* subEndpoint)
    : IPhysicEngine(pubEndpoint, subEndpoint),
      _bulletWorld(nullptr),
      _bodyManager(nullptr) {}

void BulletPhysicEngine::init() {
    _bulletWorld = new BulletWorld();
    _bulletWorld->init();
    _bodyManager = new BulletBodyManager(_bulletWorld->getWorld());

    _lastFrameTime = std::chrono::high_resolution_clock::now();

    subscribe("PhysicCommand", [this](const std::string& msg) {
        this->onPhysicCommand(msg);
    });

    std::cout << "[BulletPhysicEngine] Initialized" << std::endl;
}

void BulletPhysicEngine::cleanup() {
    if (_bodyManager) { delete _bodyManager; _bodyManager = nullptr; }
    if (_bulletWorld) { delete _bulletWorld; _bulletWorld = nullptr; }
}

void BulletPhysicEngine::loop() {
    static int heartbeat = 0;
    if (++heartbeat % 60 == 0 && _bodyManager) {
        std::cout << "[Bullet] Heartbeat - Loop Running. Bodies tracked: " << _bodyManager->getBodies().size() << std::endl;
    }

    stepSimulation();
    checkCollisions();
    sendUpdates();
}

void BulletPhysicEngine::stepSimulation() {
    if (!_bulletWorld) return;

    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsedTime = currentTime - _lastFrameTime;
    _lastFrameTime = currentTime;

    float deltaTime = elapsedTime.count();

    if (deltaTime > _maxDeltaTime) {
        deltaTime = _maxDeltaTime;
    }

    _timeAccumulator += deltaTime;
    const float fixedTimeStep = 1.0f / 60.0f;

    while (_timeAccumulator >= fixedTimeStep) {
        // Calling step with timeStep=fixedTimeStep, maxSubSteps=0 (since we loop manually)
        // Or just let bullet handle it?
        // _dynamicsWorld->stepSimulation(fixedTimeStep, 10) takes timeStep.
        _bulletWorld->step(fixedTimeStep, 10);
        _timeAccumulator -= fixedTimeStep;
    }
}

void BulletPhysicEngine::checkCollisions() {
    if (!_bulletWorld || !_bodyManager) return;
    btCollisionDispatcher* dispatcher = _bulletWorld->getDispatcher();
    if (!dispatcher) return;

    int numManifolds = dispatcher->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        if (i >= dispatcher->getNumManifolds()) break;
        btPersistentManifold* contactManifold = dispatcher->getManifoldByIndexInternal(i);
        if (!contactManifold) continue;

        const btCollisionObject* obA = contactManifold->getBody0();
        const btCollisionObject* obB = contactManifold->getBody1();

        int numContacts = contactManifold->getNumContacts();
        for (int j = 0; j < numContacts; j++) {
            btManifoldPoint& pt = contactManifold->getContactPoint(j);
            if (pt.getDistance() < 0.f) {
                const btRigidBody* bodyA = btRigidBody::upcast(obA);
                const btRigidBody* bodyB = btRigidBody::upcast(obB);

                if (bodyA && bodyB) {
                    std::string idA = _bodyManager->getBodyId(const_cast<btRigidBody*>(bodyA));
                    std::string idB = _bodyManager->getBodyId(const_cast<btRigidBody*>(bodyB));

                    if (!idA.empty() && !idB.empty()) {
                        std::stringstream ss;
                        ss << "Collision:" << idA << ":" << idB << ";";
                        sendMessage("PhysicEvent", ss.str());
                    }
                }
                // Only report one contact point per manifold to avoid spamming
                break;
            }
        }
    }
}

void BulletPhysicEngine::sendUpdates() {
    std::stringstream batchStream;
    bool hasUpdates = false;

    if (!_bodyManager) return;
    const auto& bodies = _bodyManager->getBodies();

    for (auto& pair : bodies) {
        btTransform trans;
        if (pair.second && pair.second->getMotionState()) {
            pair.second->getMotionState()->getWorldTransform(trans);
            btVector3 pos = trans.getOrigin();

            btScalar yaw, pitch, roll;
            trans.getBasis().getEulerYPR(yaw, pitch, roll);

            batchStream << "EntityUpdated:" << pair.first << ":"
               << pos.x() << "," << pos.y() << "," << pos.z() << ":"
               << pitch << "," << yaw << "," << roll << ";";
            
            hasUpdates = true;
        } else {
            std::cout << "[Bullet] DEBUG: Missing MotionState for body " << pair.first << std::endl;
        }
    }

    if (hasUpdates) {
        sendMessage("EntityUpdated", batchStream.str());
    }
}

void BulletPhysicEngine::onPhysicCommand(const std::string& message) {
    try {
        std::stringstream ss(message);
        std::string segment;

        while (std::getline(ss, segment, ';')) {
            if (segment.empty()) continue;
            size_t colonPos = segment.find(':');
            if (colonPos == std::string::npos) continue;

            std::string command = segment.substr(0, colonPos);
            std::string data = segment.substr(colonPos + 1);

            if (command == "CreateBody") {
                size_t split1 = data.find(':');
                if (split1 == std::string::npos) {
                    std::cerr << "[Bullet] ERROR: Failed to parse command: CreateBody (missing id/type)" << std::endl;
                    continue;
                }

                std::string id = data.substr(0, split1);
                std::string rest = data.substr(split1 + 1);
                size_t split2 = rest.find(':');
                if (split2 == std::string::npos) {
                    std::cerr << "[Bullet] ERROR: Failed to parse command: CreateBody (missing type/params) for id '" << id << "'" << std::endl;
                    continue;
                }

                std::string type = rest.substr(0, split2);
                std::string paramsStr = rest.substr(split2 + 1);

                std::vector<float> params;
                std::stringstream pss(paramsStr);
                std::string p;
                while (std::getline(pss, p, ',')) {
                    if (!p.empty()) {
                        params.push_back(safeStof(p));
                    }
                }
                createBody(id, type, params);
            } else if (command == "ApplyForce") {
                size_t split = data.find(':');
                if (split != std::string::npos) {
                    std::string id = data.substr(0, split);
                    std::string forceStr = data.substr(split + 1);
                    std::vector<float> force;
                    std::stringstream fss(forceStr);
                    std::string f;
                    while (std::getline(fss, f, ',')) {
                        if (!f.empty()) force.push_back(safeStof(f));
                    }
                    if (force.size() == 3) {
                        applyForce(id, force);
                    }
                }
            } else if (command == "SetTransform") {
                // SetTransform:id:x,y,z:rx,ry,rz
                size_t split1 = data.find(':');
                if (split1 != std::string::npos) {
                    std::string id = data.substr(0, split1);
                    std::string rest = data.substr(split1 + 1);
                    size_t split2 = rest.find(':');
                    if (split2 != std::string::npos) {
                        std::string posStr = rest.substr(0, split2);
                        std::string rotStr = rest.substr(split2 + 1);

                        std::vector<float> pos;
                        std::stringstream pss(posStr);
                        std::string p;
                        while (std::getline(pss, p, ',')) {
                            if (!p.empty()) pos.push_back(safeStof(p));
                        }

                        std::vector<float> rot;
                        std::stringstream rss(rotStr);
                        std::string r;
                        while (std::getline(rss, r, ',')) {
                            if (!r.empty()) rot.push_back(safeStof(r));
                        }

                        if (pos.size() == 3 && rot.size() == 3) {
                            setTransform(id, pos, rot);
                        }
                    }
                }
            } else if (command == "Raycast") {
                size_t split1 = data.find(':');
                if (split1 != std::string::npos) {
                    std::string originStr = data.substr(0, split1);
                    std::string dirStr = data.substr(split1 + 1);

                    std::vector<float> origin;
                    std::stringstream oss(originStr);
                    std::string o;
                    while (std::getline(oss, o, ',')) {
                        if (!o.empty()) origin.push_back(safeStof(o));
                    }

                    std::vector<float> dir;
                    std::stringstream dss(dirStr);
                    std::string d;
                    while (std::getline(dss, d, ',')) {
                        if (!d.empty()) dir.push_back(safeStof(d));
                    }

                    if (origin.size() == 3 && dir.size() == 3) {
                        raycast(origin, dir);
                    }
                }
            } else if (command == "SetLinearVelocity") {
                size_t split1 = data.find(':');
                if (split1 == std::string::npos) {
                    std::cerr << "[Bullet] ERROR: Failed to parse command: SetLinearVelocity (missing id)" << std::endl;
                    continue;
                }

                std::string id = data.substr(0, split1);
                std::string velStr = data.substr(split1 + 1);

                std::vector<float> vel;
                std::stringstream vss(velStr);
                std::string v;
                while (std::getline(vss, v, ',')) {
                    if (!v.empty()) vel.push_back(safeStof(v));
                }

                if (vel.size() != 3) {
                    std::cerr << "[Bullet] ERROR: Failed to parse command: SetLinearVelocity (expected 3 components) for id '" << id << "'" << std::endl;
                } else {
                    setLinearVelocity(id, vel);
                }
            } else if (command == "SetAngularVelocity") {
                size_t split1 = data.find(':');
                if (split1 != std::string::npos) {
                    std::string id = data.substr(0, split1);
                    std::string velStr = data.substr(split1 + 1);

                    std::vector<float> vel;
                    std::stringstream vss(velStr);
                    std::string v;
                    while (std::getline(vss, v, ',')) {
                        if (!v.empty()) vel.push_back(safeStof(v));
                    }

                    if (vel.size() == 3) {
                        setAngularVelocity(id, vel);
                    }
                }
            } else if (command == "SetVelocityXZ") {
                size_t split1 = data.find(':');
                if (split1 != std::string::npos) {
                    std::string id = data.substr(0, split1);
                    std::string velStr = data.substr(split1 + 1);
                    std::vector<float> vel;
                    std::stringstream vss(velStr);
                    std::string v;
                    while (std::getline(vss, v, ',')) {
                        if (!v.empty()) vel.push_back(safeStof(v));
                    }
                    if (vel.size() == 2) {
                        setVelocityXZ(id, vel[0], vel[1]);
                    }
                }
            } else if (command == "ApplyImpulse") {
                size_t split = data.find(':');
                if (split != std::string::npos) {
                    std::string id = data.substr(0, split);
                    std::string forceStr = data.substr(split + 1);
                    std::vector<float> force;
                    std::stringstream fss(forceStr);
                    std::string f;
                    while (std::getline(fss, f, ',')) {
                        if (!f.empty()) force.push_back(safeStof(f));
                    }
                    if (force.size() == 3) {
                        applyImpulse(id, force);
                    }
                }
            } else if (command == "SetAngularFactor") {
                size_t split1 = data.find(':');
                if (split1 != std::string::npos) {
                    std::string id = data.substr(0, split1);
                    std::string factorStr = data.substr(split1 + 1);
                    std::vector<float> factor;
                    std::stringstream fss(factorStr);
                    std::string f;
                    while (std::getline(fss, f, ',')) {
                        if (!f.empty()) factor.push_back(safeStof(f));
                    }
                    if (factor.size() == 3) {
                        setAngularFactor(id, factor);
                    }
                }
            } else if (command == "SetMass") {
                size_t split1 = data.find(':');
                if (split1 != std::string::npos) {
                    std::string id = data.substr(0, split1);
                    float mass = safeStof(data.substr(split1 + 1));
                    setMass(id, mass);
                }
            } else if (command == "SetFriction") {
                size_t split1 = data.find(':');
                if (split1 != std::string::npos) {
                    std::string id = data.substr(0, split1);
                    float friction = safeStof(data.substr(split1 + 1));
                    setFriction(id, friction);
                }
            } else if (command == "DestroyBody") {
                destroyBody(data);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[BulletPhysicEngine] PhysicCommand parse error: " << e.what() << " in msg='" << message << "'" << std::endl;
    }
}

void BulletPhysicEngine::destroyBody(const std::string& id) {
    if (_bodyManager) _bodyManager->destroyBody(id);
}

void BulletPhysicEngine::createBody(const std::string& id, const std::string& type, const std::vector<float>& params) {
    if (_bodyManager) _bodyManager->createBody(id, type, params);
}

void BulletPhysicEngine::applyForce(const std::string& id, const std::vector<float>& force) {
    if (_bodyManager) _bodyManager->applyForce(id, force);
}

void BulletPhysicEngine::setTransform(const std::string& id, const std::vector<float>& pos, const std::vector<float>& rot) {
    if (_bodyManager) _bodyManager->setTransform(id, pos, rot);
}

void BulletPhysicEngine::raycast(const std::vector<float>& origin, const std::vector<float>& direction) {
    if (!_bulletWorld || !_bodyManager) return;
    btDiscreteDynamicsWorld* world = _bulletWorld->getWorld();
    if (!world) return;

    btVector3 from(origin[0], origin[1], origin[2]);
    btVector3 to = from + btVector3(direction[0], direction[1], direction[2]) * 1000.0f; // Ray length 1000

    btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);
    world->rayTest(from, to, rayCallback);

    if (rayCallback.hasHit()) {
        const btRigidBody* body = btRigidBody::upcast(rayCallback.m_collisionObject);
        if (body) {
            std::string id = _bodyManager->getBodyId(const_cast<btRigidBody*>(body));
            if (!id.empty()) {
                std::stringstream ss;
                // RaycastHit:id:distance
                float dist = (rayCallback.m_hitPointWorld - from).length();
                ss << "RaycastHit:" << id << ":" << dist << ";";
                sendMessage("PhysicEvent", ss.str());
            }
        }
    }
}

void BulletPhysicEngine::setLinearVelocity(const std::string& id, const std::vector<float>& vel) {
    if (_bodyManager) _bodyManager->setLinearVelocity(id, vel);
}

void BulletPhysicEngine::setAngularVelocity(const std::string& id, const std::vector<float>& vel) {
    if (_bodyManager) _bodyManager->setAngularVelocity(id, vel);
}

void BulletPhysicEngine::setMass(const std::string& id, float mass) {
    if (_bodyManager) _bodyManager->setMass(id, mass);
}

void BulletPhysicEngine::setFriction(const std::string& id, float friction) {
    if (_bodyManager) _bodyManager->setFriction(id, friction);
}

void BulletPhysicEngine::setVelocityXZ(const std::string& id, float vx, float vz) {
    if (_bodyManager) _bodyManager->setVelocityXZ(id, vx, vz);
}

void BulletPhysicEngine::applyImpulse(const std::string& id, const std::vector<float>& impulse) {
    if (_bodyManager) _bodyManager->applyImpulse(id, impulse);
}

void BulletPhysicEngine::setAngularFactor(const std::string& id, const std::vector<float>& factor) {
    if (_bodyManager) _bodyManager->setAngularFactor(id, factor);
}

} // namespace rtypeEngine

#ifdef _WIN32
    #define BULLET_PHYSIC_ENGINE_EXPORT __declspec(dllexport)
#else
    #define BULLET_PHYSIC_ENGINE_EXPORT
#endif

extern "C" BULLET_PHYSIC_ENGINE_EXPORT rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::BulletPhysicEngine(pubEndpoint, subEndpoint);
}
