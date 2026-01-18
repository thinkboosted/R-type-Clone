#pragma once
#include <btBulletDynamicsCommon.h>

namespace rtypeEngine {
    class BulletWorld {
    public:
        BulletWorld();
        ~BulletWorld();

        void init();
        void cleanup();
        void step(float deltaTime, int maxSubSteps = 10, float fixedTimeStep = 1.0f/60.0f);

        btDiscreteDynamicsWorld* getWorld() const { return _dynamicsWorld; }
        btCollisionDispatcher* getDispatcher() const { return _dispatcher; }

    private:
        btDefaultCollisionConfiguration* _collisionConfiguration;
        btCollisionDispatcher* _dispatcher;
        btBroadphaseInterface* _overlappingPairCache;
        btSequentialImpulseConstraintSolver* _solver;
        btDiscreteDynamicsWorld* _dynamicsWorld;
    };
}
