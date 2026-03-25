#pragma once
#include "IPlayerAvatarAnimator.h"
#include "../../../../../../Engine/Module/Component/Animator/Animator.h"

namespace GameCore::PlayerAvatar
{
    template <typename T>
    concept AnimationEnum = std::is_enum_v<T> && std::is_integral_v<std::underlying_type_t<T>>;
    
    template <AnimationEnum AnimationT>
    class PlayerAvatarAnimator final : public IPlayerAvatarAnimator
    {
    public:
        explicit PlayerAvatarAnimator(std::weak_ptr<Component::Animator>& animator);
        void ChangeAnimation(AnimationT animationType);
        void OnDrawGui() override;

    private:
        const std::string stateParamName_ = "State"; 
        std::weak_ptr<Component::Animator>& animator_;
    };

    template <AnimationEnum AnimationT>
    PlayerAvatarAnimator<AnimationT>::PlayerAvatarAnimator(
        std::weak_ptr<Component::Animator>& animator)
        : animator_(animator)
    {
        
    }

    template <AnimationEnum AnimationT>
    void PlayerAvatarAnimator<AnimationT>::ChangeAnimation(AnimationT animationType)
    {
        animator_.lock()->Param<int>(stateParamName_).Set(static_cast<int>(animationType));
    }

    template <AnimationEnum AnimationT>
    void PlayerAvatarAnimator<AnimationT>::OnDrawGui()
    {
        
    }
}
