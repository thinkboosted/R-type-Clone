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
      _collisionConfiguration(nullptr),
      _dispatcher(nullptr),
      _overlappingPairCache(nullptr),
      _solver(nullptr),
      _dynamicsWorld(nullptr) {}

void BulletPhysicEngine::init() {
    _collisionConfiguration = new btDefaultCollisionConfiguration();
    _dispatcher = new btCollisionDispatcher(_collisionConfiguration);
    _overlappingPairCache = new btDbvtBroadphase();
    _solver = new btSequentialImpulseConstraintSolver;
    _dynamicsWorld = new btDiscreteDynamicsWorld(_dispatcher, _overlappingPairCache, _solver, _collisionConfiguration);

    _lastFrameTime = std::chrono::high_resolution_clock::now();
    // Need to be changed in lua
    _dynamicsWorld->setGravity(btVector3(0, 0, 0));

    subscribe("PhysicCommand", [this](const std::string& msg) {
        this->onPhysicCommand(msg);
    });

    std::cout << "[BulletPhysicEngine] Initialized" << std::endl;
}

void BulletPhysicEngine::cleanup() {
    for (auto& pair : _bodies) {
        _dynamicsWorld->removeRigidBody(pair.second);
        delete pair.second->getMotionState();
        delete pair.second->getCollisionShape();
        delete pair.second;
    }
    _bodies.clear();
    _bodyIds.clear();

    if (_dynamicsWorld) { delete _dynamicsWorld; _dynamicsWorld = nullptr; }
    if (_solver) { delete _solver; _solver = nullptr; }
    if (_overlappingPairCache) { delete _overlappingPairCache; _overlappingPairCache = nullptr; }
    if (_dispatcher) { delete _dispatcher; _dispatcher = nullptr; }
    if (_collisionConfiguration) { delete _collisionConfiguration; _collisionConfiguration = nullptr; }
}

void BulletPhysicEngine::loop() {
    static int heartbeat = 0;
    if (++heartbeat % 60 == 0) {
        std::cout << "[Bullet] Heartbeat - Loop Running. Bodies tracked: " << _bodies.size() << std::endl;
    }

    stepSimulation();
    checkCollisions();
    sendUpdates();
}

void BulletPhysicEngine::stepSimulation() {
    if (!_dynamicsWorld) return;

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
        _dynamicsWorld->stepSimulation(fixedTimeStep, 10);
        _timeAccumulator -= fixedTimeStep;
    }
}

void BulletPhysicEngine::checkCollisions() {
    if (!_dispatcher) return;

    int numManifolds = _dispatcher->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        if (i >= _dispatcher->getNumManifolds()) break;
        btPersistentManifold* contactManifold = _dispatcher->getManifoldByIndexInternal(i);
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
                    auto itA = _bodyIds.find(const_cast<btRigidBody*>(bodyA));
                    auto itB = _bodyIds.find(const_cast<btRigidBody*>(bodyB));

                    if (itA != _bodyIds.end() && itB != _bodyIds.end()) {
                        std::stringstream ss;
                        ss << "Collision:" << itA->second << ":" << itB->second << ";";
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

    for (auto& pair : _bodies) {
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
                    continue;
                }

                setLinearVelocity(id, vel);
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

void BulletPhysicEngine::createBody(const std::string& id, const std::string& type, const std::vector<float>& params) {
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

void BulletPhysicEngine::applyForce(const std::string& id, const std::vector<float>& force) {
    if (_bodies.find(id) != _bodies.end()) {
        _bodies[id]->activate(true);
        _bodies[id]->applyCentralForce(btVector3(force[0], force[1], force[2]));
    }
}

void BulletPhysicEngine::setTransform(const std::string& id, const std::vector<float>& pos, const std::vector<float>& rot) {
    if (_bodies.find(id) != _bodies.end()) {
        btRigidBody* body = _bodies[id];
        btTransform trans;
        trans.setIdentity();
        trans.setOrigin(btVector3(pos[0], pos[1], pos[2]));

        float degToRad = static_cast<float>(M_PI) / 180.0f;
        btQuaternion quat;
        // setEulerZYX expects Yaw (Y), Pitch (X), Roll (Z) in radians
        // rot is rx (X), ry (Y), rz (Z) in degrees
        quat.setEulerZYX(rot[1] * degToRad, rot[0] * degToRad, rot[2] * degToRad);
        trans.setRotation(quat);

        body->setWorldTransform(trans);
        if (body->getMotionState()) {
            body->getMotionState()->setWorldTransform(trans);
        }
        body->activate(true);
    }
}

void BulletPhysicEngine::raycast(const std::vector<float>& origin, const std::vector<float>& direction) {
    if (!_dynamicsWorld) return;

    btVector3 from(origin[0], origin[1], origin[2]);
    btVector3 to = from + btVector3(direction[0], direction[1], direction[2]) * 1000.0f; // Ray length 1000

    btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);
    _dynamicsWorld->rayTest(from, to, rayCallback);

    if (rayCallback.hasHit()) {
        const btRigidBody* body = btRigidBody::upcast(rayCallback.m_collisionObject);
        if (body) {
            auto it = _bodyIds.find(const_cast<btRigidBody*>(body));
            if (it != _bodyIds.end()) {
                std::stringstream ss;
                // RaycastHit:id:distance
                float dist = (rayCallback.m_hitPointWorld - from).length();
                ss << "RaycastHit:" << it->second << ":" << dist << ";";
                sendMessage("PhysicEvent", ss.str());
            }
        }
    }
}

void BulletPhysicEngine::setLinearVelocity(const std::string& id, const std::vector<float>& vel) {
    auto it = _bodies.find(id);
    if (it == _bodies.end()) {
        std::cerr << "[Bullet] ERROR: SetLinearVelocity failed. Entity ID '" << id << "' does not exist in Physics World." << std::endl;
        return;
    }
    it->second->activate(true);
    it->second->setLinearVelocity(btVector3(vel[0], vel[1], vel[2]));
}

void BulletPhysicEngine::setAngularVelocity(const std::string& id, const std::vector<float>& vel) {
    auto it = _bodies.find(id);
    if (it == _bodies.end()) {
        std::cerr << "[Bullet] ERROR: SetAngularVelocity failed. Entity ID '" << id << "' does not exist in Physics World." << std::endl;
        return;
    }
    it->second->activate(true);
    it->second->setAngularVelocity(btVector3(vel[0], vel[1], vel[2]));
}

void BulletPhysicEngine::setMass(const std::string& id, float mass) {
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

void BulletPhysicEngine::setFriction(const std::string& id, float friction) {
    if (_bodies.find(id) != _bodies.end()) {
        _bodies[id]->setFriction(friction);
        _bodies[id]->activate(true);
    }
}

void BulletPhysicEngine::setVelocityXZ(const std::string& id, float vx, float vz) {
    if (_bodies.find(id) != _bodies.end()) {
        btRigidBody* body = _bodies[id];
        body->activate(true);
        btVector3 vel = body->getLinearVelocity();
        vel.setX(vx);
        vel.setZ(vz);
        body->setLinearVelocity(vel);
    }
}

void BulletPhysicEngine::applyImpulse(const std::string& id, const std::vector<float>& impulse) {
    if (_bodies.find(id) != _bodies.end()) {
        _bodies[id]->activate(true);
        _bodies[id]->applyCentralImpulse(btVector3(impulse[0], impulse[1], impulse[2]));
    }
}

void BulletPhysicEngine::setAngularFactor(const std::string& id, const std::vector<float>& factor) {
    if (_bodies.find(id) != _bodies.end()) {
        _bodies[id]->activate(true);
        _bodies[id]->setAngularFactor(btVector3(factor[0], factor[1], factor[2]));
    }
}

} // namespace rtypeEngine

extern "C" rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::BulletPhysicEngine(pubEndpoint, subEndpoint);
}
