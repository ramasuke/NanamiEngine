#include "Time.h"
#include "DxLib.h"

namespace NanamiEngine
{
    long long Time::lastTime_    = 0;
    float Time::deltaTime_       = 0.0f;
    float Time::timeScale_       = 1.0f;
    float Time::currentTime_     = 0.0f;
    int   Time::isSkipNextFrame_ = 0;
    float Time::fixedAlpha_      = 0.0f;
    float Time::fixedDeltaTime_  = 1.0f / 60.0f;

    void Time::Update()
    {
        if (isSkipNextFrame_ > 0)
        {
            isSkipNextFrame_--;
        }
        
        // NOTE: ms 単位だと 16/17ms の揺れで固定ステップ数と補間の alpha がぶれるので µs で測る
        const long long now = GetNowHiPerformanceCount();

        if (lastTime_ == 0)
        {
            lastTime_ = now;
            deltaTime_ = 0.0f;
            return;
        }

        const float rawDelta = static_cast<float>(static_cast<double>(now - lastTime_) / 1'000'000.0);

        deltaTime_ = rawDelta * timeScale_;
        currentTime_ += deltaTime_;

        lastTime_ = now;
    }

    float Time::DeltaTime()
    {
        if (isSkipNextFrame_ > 0)
            return 0.0f;
        
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

    float Time::GetFixedAlpha()
    {
        return fixedAlpha_;
    }

    void Time::SetFixedAlpha(const float alpha)
    {
        fixedAlpha_ = alpha;
    }

    float Time::FixedDeltaTime()
    {
        return fixedDeltaTime_;
    }

    void Time::SetFixedDeltaTime(const float fixedDeltaTime)
    {
        fixedDeltaTime_ = fixedDeltaTime;
    }
}