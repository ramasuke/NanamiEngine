#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Core::Application::Configuration
{
    class NANAMI_API PhysicsConfiguration final
    {
    public:
        static void Load();
        static void Save();

        [[nodiscard]] static int   GetFixedUpdateRate();
        static void                SetFixedUpdateRate(int rate);

        [[nodiscard]] static float GetMaxDeltaTime();
        static void                SetMaxDeltaTime(float t);

        [[nodiscard]] static int   GetMaxPhysicsStep();
        static void                SetMaxPhysicsStep(int step);

        [[nodiscard]] static float GetGravityScale();
        static void                SetGravityScale(float scale);

        [[nodiscard]] static int   GetCollisionSteps();
        static void                SetCollisionSteps(int steps);

        // 面に沿ってほぼ止まっている接触に使う合成摩擦。0なら静止摩擦自体を使わない
        [[nodiscard]] static float GetStaticFriction();
        static void                SetStaticFriction(float friction);

        // これ未満の面方向相対速度を「止まっている」とみなす
        [[nodiscard]] static float GetStaticFrictionSpeed();
        static void                SetStaticFrictionSpeed(float speed);

        // これより急な面は静止摩擦の対象外にして、従来どおり滑らせる
        [[nodiscard]] static float GetStaticFrictionMaxSlopeDeg();
        static void                SetStaticFrictionMaxSlopeDeg(float degree);

        [[nodiscard]] static int   GetTempAllocatorSizeMB();
        static void                SetTempAllocatorSizeMB(int sizeMB);

        [[nodiscard]] static int   GetMaxBodies();
        static void                SetMaxBodies(int maxBodies);

        [[nodiscard]] static int   GetMaxBodyPairs();
        static void                SetMaxBodyPairs(int maxBodyPairs);

        [[nodiscard]] static int   GetMaxContactConstraints();
        static void                SetMaxContactConstraints(int maxConstraints);

        static void DrawConfigGUI();

    private:
        static int   fixedUpdateRate_;
        static float maxDeltaTime_;
        static int   maxPhysicsStep_;

        static float gravityScale_;
        static int   collisionSteps_;
        static float staticFriction_;
        static float staticFrictionSpeed_;
        static float staticFrictionMaxSlopeDeg_;
        static int   tempAllocatorSizeMB_;
        static int   maxBodies_;
        static int   maxBodyPairs_;
        static int   maxContactConstraints_;
    };
}
