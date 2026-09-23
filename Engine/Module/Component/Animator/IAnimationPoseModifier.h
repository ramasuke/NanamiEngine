#pragma once

namespace NanamiEngine::Module::Component
{
    /** @brief 同じ GameObject の Animator がアニメーションを適用した直後に呼ばれ、ボーン姿勢を上書きできる */
    class IAnimationPoseModifier
    {
    public:
        virtual ~IAnimationPoseModifier() = default;
        virtual void OnModifyPose(int modelHandle) = 0;
    };
}
