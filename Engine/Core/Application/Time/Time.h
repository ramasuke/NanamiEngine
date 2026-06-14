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
        static void SkipNextFrame();
        static float GetTimeScale();
        static float GetFixedAlpha();
        static void  SetFixedAlpha(float alpha);

    private:
        static int lastTime_;
        static float deltaTime_;
        static float timeScale_;
        static float currentTime_;
        static int   isSkipNextFrame_;
        static float fixedAlpha_;
    };
}
