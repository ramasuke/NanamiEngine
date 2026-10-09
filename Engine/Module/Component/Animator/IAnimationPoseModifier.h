#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Module::Component
{
    // NOTE: 同じ GameObject のアニメーション適用直後に呼ばれ、ボーン姿勢を上書きできる
    class NANAMI_API IAnimationPoseModifier
    {
    public:
        virtual ~IAnimationPoseModifier() = default;
        virtual void OnModifyPose(int modelHandle) = 0;
    };
}
