#pragma once

namespace NanamiEngine
{
    class Time final
    {
    public:
        static void Update();
        static float DeltaTime();
        static float CurrentTime();
        static void SetTimeScale(float scale);
        static float GetTimeScale();

    private:
        static int lastTime_;
        static float deltaTime_;
        static float timeScale_;
        static float currentTime_;
    };
}
