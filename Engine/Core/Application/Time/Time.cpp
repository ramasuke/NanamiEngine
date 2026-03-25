#include "Time.h"
#include "DxLib.h"

int   Time::lastTime_   = 0;
float Time::deltaTime_  = 0.0f;
float Time::timeScale_  = 1.0f;

void Time::Update()
{
    const int now = GetNowCount();
    const float rawDelta = (now - lastTime_) / 1000.0f;
    deltaTime_ = rawDelta * timeScale_;
    lastTime_ = now;
}

float Time::DeltaTime()
{
    return deltaTime_;
}

void Time::SetTimeScale(const float scale)
{
    timeScale_ = scale;
}

float Time::GetTimeScale()
{
    return timeScale_;
}