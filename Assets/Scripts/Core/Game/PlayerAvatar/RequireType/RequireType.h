#pragma once
#include <type_traits>

#include "../Animator/PlayerAvatarAnimator.h"
#include "../CameraGroup/PlayerAvatarCameraGroupBase.h"
#include "../InputAction/PlayerAvatarInputActionBase.h"
#include "../State/IPlayerAvatarState.h"
#include "../StateMachine/IPlayerAvatarStateMachine.h"
#include "../Status/IPlayerAvatarStatus.h"

namespace GameCore::PlayerAvatar::RequireType
{
    template <typename Traits>
        requires std::derived_from<typename Traits::Animator, IPlayerAvatarAnimator>
    using Animator = typename Traits::Animator;
    template <typename Traits>
        requires std::derived_from<typename Traits::Status, IPlayerAvatarStatus>
    using Status = typename Traits::Status;
    template <typename Traits>
        requires std::derived_from<typename Traits::StateMachine, IPlayerAvatarStateMachine>
    using StateMachine = typename Traits::StateMachine;
    template <typename Traits>
        requires std::derived_from<typename Traits::State, IPlayerAvatarState>
    using State = typename Traits::State;
    template <typename Traits>
        requires std::derived_from<typename Traits::InputAction, PlayerAvatarInputActionBase>
    using InputAction = typename Traits::InputAction;
    template <typename Traits>
        requires std::derived_from<typename Traits::CameraGroup, PlayerAvatarCameraGroupBase>
    using CameraGroup = typename Traits::CameraGroup;

    template <typename Traits>  
        concept HasAnimator = requires { typename Animator<Traits>; };
    template <typename Traits>
        concept HasStatus = requires { typename Status<Traits>; };
    template <typename Traits>
        concept HasStateMachine = requires { typename StateMachine<Traits>; };
    template <typename Traits>
        concept HasState = requires { typename State<Traits>; };
    template <typename Traits>
        concept HasInputAction = requires { typename InputAction<Traits>; };
    template <typename Traits>
        concept HasCameraGroup = requires { typename CameraGroup<Traits>; };
    
    
    template <typename TraitsT>
    concept Traits = RequireType::HasAnimator    <TraitsT> &&
                     RequireType::HasStatus      <TraitsT> &&
                     RequireType::HasStateMachine<TraitsT> &&
                     RequireType::HasState       <TraitsT> &&
                     RequireType::HasInputAction <TraitsT> &&
                     RequireType::HasCameraGroup <TraitsT>;
}
