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
        // 物理の固定ステップ1回分の秒数(OnFixedUpdate / OnBeginPhysics 中はこちらを使う)
        static float FixedDeltaTime();
        static void  SetFixedDeltaTime(float fixedDeltaTime);

    private:
        static long long lastTime_;
        static float deltaTime_;
        static float timeScale_;
        static float currentTime_;
        static int   isSkipNextFrame_;
        static float fixedAlpha_;
        static float fixedDeltaTime_;
    };
}
