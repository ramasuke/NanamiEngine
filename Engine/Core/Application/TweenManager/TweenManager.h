#pragma once
#include "../tweeny/Tweeny/tweeny.h"

namespace NanamiEngine::Core
{
    class TweenManager final
    {
    public:
        void Tick();
        // void AddTween();
        
    private:
        std::vector<std::pair<tweeny::tween<float, float, float>, float>> tweens_;
    };
}
