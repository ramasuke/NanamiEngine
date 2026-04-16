#pragma once

class Time
{
public:
    static void Update();
    static float DeltaTime();
    static void SetTimeScale(float scale);
    static float GetTimeScale();

private:
    static int   lastTime_;
    static float deltaTime_;
    static float timeScale_;
};