#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <../JoltPhysics/Jolt/Jolt.h>
#include <../JoltPhysics/Jolt/Physics/Collision/ContactListener.h>

#include "CollisionEnterGroup/Engine_Physics_CollisionEnterGroup.h"
#include "CollisionExitGroup/Engine_Physics_CollisionExitGroup.h"
#include "ContactedData/Engine_Physics_ContactedData.h"
#include "SensorEnterGroup/Engine_Physics_SensorEnterGroup.h"
#include "SensorExitGroup/Engine_Physics_SensorExitGroup.h"

namespace JPH
{
    class PhysicsSystem;
}

namespace NanamiEngine::Module::Physics
{
    class NANAMI_API EngineContactListener final : public JPH::ContactListener
    {
    public:
        explicit EngineContactListener(const JPH::PhysicsSystem& physicsSystem);
        ~EngineContactListener() override;
        //NOTE: メインスレッド呼び出しを推奨
        void OnUpdate();
        //NOTE: 接触コールバックは物理ジョブから並列に呼ばれるので、設定値はステップ前にここへ取り込んでおく
        void RefreshTuning();
        
        void UnSubscribeEngineCollider(const JPH::BodyID& colliderId);

    private:
        void OnContactAdded(
            const JPH::Body& body1,
            const JPH::Body& body2,
            const JPH::ContactManifold& manifold,
            JPH::ContactSettings& settings) override;
        void OnContactPersisted(
            const JPH::Body& body1,
            const JPH::Body& body2,
            const JPH::ContactManifold& manifold,
            JPH::ContactSettings& settings) override;
        void OnContactRemoved(const JPH::SubShapeIDPair& pair) override;

        /**
         * @brief 面に沿ってほぼ止まっている接触だけ、摩擦を静止摩擦に差し替える
         * @note Jolt の摩擦は係数1つだけで静止/動の区別がないため、ここで切り替えないと斜面で滑り落ちる
         */
        void ApplyStaticFriction(
            const JPH::Body& body1,
            const JPH::Body& body2,
            const JPH::ContactManifold& manifold,
            JPH::ContactSettings& settings) const;

        CollisionEnterGroup collisionEnterGroup_;
        CollisionExitGroup  collisionExitGroup_;
        SensorEnterGroup    sensorEnterGroup_;
        SensorExitGroup     sensorExitGroup_;
        const JPH::PhysicsSystem& physicsSystem_;
        float staticFriction_      = 0.0f;
        float staticFrictionSpeed_ = 0.0f;
        float cosMaxSlope_         = 1.0f;
    };
}