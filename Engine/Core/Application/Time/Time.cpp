#include "Time.h"
#include "DxLib.h"

namespace NanamiEngine
{
    int   Time::lastTime_        = 0;
    float Time::deltaTime_       = 0.0f;
    float Time::timeScale_       = 1.0f;
    float Time::currentTime_     = 0.0f;
    int   Time::isSkipNextFrame_ = 0;

    void Time::Update()
    {
        isSkipNextFrame_--;
        
        const int now = GetNowCount();

        if (lastTime_ == 0)
        {
            lastTime_ = now;
            deltaTime_ = 0.0f;
            return;
        }

        const float rawDelta = (now - lastTime_) / 1000.0f;

        deltaTime_ = rawDelta * timeScale_;
        currentTime_ += deltaTime_;

        lastTime_ = now;
    }

    float Time::DeltaTime()
    {
        if (isSkipNextFrame_ > 0)
        {
            return 0.0f;
        }
        
        return deltaTime_;
    }

    float Time::CurrentTime()
    {
        return currentTime_;
    }

    void Time::SetTimeScale(const float scale)
    {
        timeScale_ = scale;
    }

    void Time::SkipNextFrame()
    {
        isSkipNextFrame_++;
    }

    float Time::GetTimeScale()
    {
        return timeScale_;
    }
}