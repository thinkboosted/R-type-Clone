#include "BulletWorld.hpp"
#include <iostream>

namespace rtypeEngine {

    BulletWorld::BulletWorld()
        : _collisionConfiguration(nullptr),
          _dispatcher(nullptr),
          _overlappingPairCache(nullptr),
          _solver(nullptr),
          _dynamicsWorld(nullptr) {}

    BulletWorld::~BulletWorld() {
        cleanup();
    }

    void BulletWorld::init() {
        _collisionConfiguration = new btDefaultCollisionConfiguration();
        _dispatcher = new btCollisionDispatcher(_collisionConfiguration);
        _overlappingPairCache = new btDbvtBroadphase();
        _solver = new btSequentialImpulseConstraintSolver;
        _dynamicsWorld = new btDiscreteDynamicsWorld(_dispatcher, _overlappingPairCache, _solver, _collisionConfiguration);

        // Standard gravity (Y-down for 3D games, disabled for 2D space-shooter)
        _dynamicsWorld->setGravity(btVector3(0, -9.81, 0));
    }

    void BulletWorld::cleanup() {
        if (_dynamicsWorld) { delete _dynamicsWorld; _dynamicsWorld = nullptr; }
        if (_solver) { delete _solver; _solver = nullptr; }
        if (_overlappingPairCache) { delete _overlappingPairCache; _overlappingPairCache = nullptr; }
        if (_dispatcher) { delete _dispatcher; _dispatcher = nullptr; }
        if (_collisionConfiguration) { delete _collisionConfiguration; _collisionConfiguration = nullptr; }
    }

    void BulletWorld::step(float deltaTime, int maxSubSteps, float fixedTimeStep) {
        if (_dynamicsWorld) {
            _dynamicsWorld->stepSimulation(deltaTime, maxSubSteps, fixedTimeStep);
        }
    }
}
