#pragma once

namespace NanamiEngine::Core::Application::Configuration
{
    class PhysicsConfiguration final
    {
    public:
        static void Load();
        static void Save();

        [[nodiscard]] static float GetGravityScale();
        static void                SetGravityScale(float scale);

        [[nodiscard]] static int   GetCollisionSteps();
        static void                SetCollisionSteps(int steps);

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
        static float gravityScale_;
        static int   collisionSteps_;
        static int   tempAllocatorSizeMB_;
        static int   maxBodies_;
        static int   maxBodyPairs_;
        static int   maxContactConstraints_;
    };
}
