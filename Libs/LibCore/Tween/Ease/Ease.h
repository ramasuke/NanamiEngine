#pragma once
#include "Functor/EaseFunctor.h"

namespace LibCore::Tween
{
    inline EaseFunctor Ease(const EaseType easing)
    {
        return EaseFunctor{ easing };
    }

    // Back系の行き過ぎ量を変える
    inline EaseFunctor Ease(const EaseType easing, const float overshoot)
    {
        return EaseFunctor{ easing, overshoot };
    }
}
