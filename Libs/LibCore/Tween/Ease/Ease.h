#pragma once
#include "Functor/EaseFunctor.h"

namespace LibCore::Tween
{
    inline EaseFunctor Ease(const EaseType easing)
    {
        return EaseFunctor{ easing };
    }
}
