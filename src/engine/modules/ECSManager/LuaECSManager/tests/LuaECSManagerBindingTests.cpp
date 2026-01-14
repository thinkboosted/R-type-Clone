/**
 * @file LuaECSManagerBindingTests.cpp
 * @brief Unit tests for Lua-C++ binding verification
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <sol/sol.hpp>

#include "LuaECSManager.hpp"
#include "../../../core/Logger.hpp"

namespace rtypeEngine {

class LuaECSManagerBindingTest : public ::testing::Test {
protected:
    static constexpr const char* PUB_ENDPOINT = "inproc://test_pub";
    static constexpr const char* SUB_ENDPOINT = "inproc://test_sub";

    void SetUp() override {
        manager = std::make_shared<LuaECSManager>(PUB_ENDPOINT, SUB_ENDPOINT);
        manager->setSelfReference(manager);
        manager->init();
    }

    void TearDown() override {
        if (manager) {
            manager->cleanup();
        }
    }

    std::shared_ptr<LuaECSManager> manager;
};

// TEST 1: Transform Binding Registration
TEST_F(LuaECSManagerBindingTest, TransformBindingRegistered) {
    sol::state& lua = manager->getLuaState();
    EXPECT_NO_THROW({
        lua.script(R"(
            local t = Transform(1, 2, 3)
            assert(t.x == 1)
            assert(t.y == 2)
            assert(t.z == 3)
        )");
    });
}

// TEST 2: Transform Helper Methods
TEST_F(LuaECSManagerBindingTest, TransformHelperMethods) {
    sol::state& lua = manager->getLuaState();
    EXPECT_NO_THROW({
        lua.script(R"(
            local t = Transform(0, 0, 0)
            t:setPosition(10, 20, 30)
            assert(t.x == 10)
            assert(t.y == 20)
            assert(t.z == 30)
            t:setScale(2)
            assert(t.sx == 2)
            assert(t.sy == 2)
            assert(t.sz == 2)
        )");
    });
}

// TEST 3: Collider Binding Registration
TEST_F(LuaECSManagerBindingTest, ColliderBindingRegistered) {
    sol::state& lua = manager->getLuaState();
    EXPECT_NO_THROW({
        lua.script(R"(
            local c = Collider("Box", 1, 1, 1)
            assert(c:getTypeString() == "Box")
            assert(c:isValid() == true)
        )");
    });
}

// TEST 4: Collider Type Validation
TEST_F(LuaECSManagerBindingTest, ColliderTypeValidation) {
    sol::state& lua = manager->getLuaState();
    EXPECT_NO_THROW({
        lua.script(R"(
            local c1 = Collider("box", 1, 1, 1)
            assert(c1:getTypeString() == "Box")
            local c2 = Collider("SPHERE", 2)
            assert(c2:getTypeString() == "Sphere")
        )");
    });
}

// TEST 5: Mesh Binding Registration
TEST_F(LuaECSManagerBindingTest, MeshBindingRegistered) {
    sol::state& lua = manager->getLuaState();
    EXPECT_NO_THROW({
        lua.script(R"(
            local m = Mesh("assets/models/cube.obj")
            assert(m.modelPath == "assets/models/cube.obj")
        )");
    });
}

// TEST 6: Vec3 Still Works
TEST_F(LuaECSManagerBindingTest, Vec3StillWorks) {
    sol::state& lua = manager->getLuaState();
    EXPECT_NO_THROW({
        lua.script(R"(
            local v = Vec3(1, 2, 3)
            assert(v.x == 1)
            assert(v.y == 2)
            assert(v.z == 3)
            local v2 = Vec3(3, 4, 0)
            assert(v2:length() == 5)
        )");
    });
}

// TEST 7: Weak Pointer Safety
TEST_F(LuaECSManagerBindingTest, WeakPointerExpires) {
    std::weak_ptr<LuaECSManager> weakRef;
    {
        auto mgr = std::make_shared<LuaECSManager>(PUB_ENDPOINT, SUB_ENDPOINT);
        mgr->setSelfReference(mgr);
        mgr->init();
        weakRef = mgr->getWeakSelfRef();
        EXPECT_FALSE(weakRef.expired());
        mgr->cleanup();
    }
    EXPECT_TRUE(weakRef.expired());
}

// TEST 8: Entity Creation Works
TEST_F(LuaECSManagerBindingTest, EntityCreationWorks) {
    sol::state& lua = manager->getLuaState();
    EXPECT_NO_THROW({
        lua.script(R"(
            local ent = ECS.createEntity()
            assert(ent ~= nil)
            assert(type(ent) == "string")
        )");
    });
}

// TEST 9: Transform Component Addition
TEST_F(LuaECSManagerBindingTest, TransformComponentAddition) {
    sol::state& lua = manager->getLuaState();
    EXPECT_NO_THROW({
        lua.script(R"(
            local ent = ECS.createEntity()
            local t = Transform(5, 10, 15)
            ECS.addComponent(ent, "Transform", t)
            local retrieved = ECS.getComponent(ent, "Transform")
            assert(retrieved ~= nil)
        )");
    });
}

// TEST 10: Collider Component Addition
TEST_F(LuaECSManagerBindingTest, ColliderComponentAddition) {
    sol::state& lua = manager->getLuaState();
    EXPECT_NO_THROW({
        lua.script(R"(
            local ent = ECS.createEntity()
            local c = Collider("Box", 1, 2, 3)
            ECS.addComponent(ent, "Collider", c)
            local retrieved = ECS.getComponent(ent, "Collider")
            assert(retrieved ~= nil)
        )");
    });
}

// TEST 11: Mesh Component Addition
TEST_F(LuaECSManagerBindingTest, MeshComponentAddition) {
    sol::state& lua = manager->getLuaState();
    EXPECT_NO_THROW({
        lua.script(R"(
            local ent = ECS.createEntity()
            local m = Mesh("test.obj", "test.png")
            ECS.addComponent(ent, "Mesh", m)
            local retrieved = ECS.getComponent(ent, "Mesh")
            assert(retrieved ~= nil)
        )");
    });
}

// TEST 12: Multiple Entities with Components
TEST_F(LuaECSManagerBindingTest, MultipleEntitiesWithComponents) {
    sol::state& lua = manager->getLuaState();
    EXPECT_NO_THROW({
        lua.script(R"(
            local ent1 = ECS.createEntity()
            ECS.addComponent(ent1, "Transform", Transform(0, 0, 0))
            ECS.addComponent(ent1, "Collider", Collider("Box", 1, 1, 1))
            local ent2 = ECS.createEntity()
            ECS.addComponent(ent2, "Transform", Transform(10, 10, 10))
            ECS.addComponent(ent2, "Mesh", Mesh("model.obj"))
            local t1 = ECS.getComponent(ent1, "Transform")
            local t2 = ECS.getComponent(ent2, "Transform")
            assert(t1.x == 0)
            assert(t2.x == 10)
        )");
    });
}

// TEST 13: Transform Distance Calculation
TEST_F(LuaECSManagerBindingTest, TransformDistanceCalculation) {
    sol::state& lua = manager->getLuaState();
    EXPECT_NO_THROW({
        lua.script(R"(
            local t1 = Transform(0, 0, 0)
            local t2 = Transform(3, 4, 0)
            local dist = t1:getDistance(t2)
            assert(math.abs(dist - 5.0) < 0.01)
        )");
    });
}

} // namespace rtypeEngine


