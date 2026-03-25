#include "TweenManager.h"
#include "../Time/Time.h"

void NanamiEngine::Core::TweenManager::Tick()
{
    const float deltaMsecs = Time::DeltaTime() * 1000.0f;

    std::erase_if(tweens_,
                  [deltaMsecs](auto& tweenPair) {
                      auto& [tween, count_msecs] = tweenPair;
                      count_msecs += deltaMsecs;
                      tween.step(static_cast<int>(deltaMsecs));
                      return tween.progress() >= 1.0f;
                  });
}

// void NanamiEngine::Core::TweenManager::AddTween()
// {
//     tweens_.emplace_back(add, 0);
// }
