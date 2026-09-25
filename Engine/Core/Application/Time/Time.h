#pragma once

namespace NanamiEngine
{
    class Time final
    {
    public:
        static void Update();
        static float DeltaTime();
        static float CurrentTime();
        /** @brief 起動からの経過ミリ秒 (壁時計。DeltaTime が止まるロード中の待ちに使う)。int なのでいつか折り返す */
        static int   NowMilliseconds();
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
