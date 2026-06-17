#pragma once
#include <memory>
#include <unordered_map>
#include <functional>
#include "ImGuiHelper.h"
#include "IPlayerAvatarStateMachine.h"
#include "../State/IPlayerAvatarState.h"
#include "../rxcpp/subjects/rx-behavior.hpp"
#include "EventScene/IPlayerAvatarEventSceneStateMachine.h"

namespace GameCore::PlayerAvatar
{
    class PlayerAvatarStateMachine : public IPlayerAvatarStateMachine,
                                     public IPlayerAvatarEventSceneStateMachine
    {
    public:
        using StateMap              = std::unordered_map<SwordMan::SwordManAvatarStateType, std::shared_ptr<IPlayerAvatarState>>;
        using OnChangeStateCallback = std::function<void(SwordMan::SwordManAvatarStateType)>;
        using StatesFactory         = std::function<StateMap(OnChangeStateCallback)>;

        explicit PlayerAvatarStateMachine(
            StatesFactory factory,
            SwordMan::SwordManAvatarStateType initialState,
            SwordMan::SwordManAvatarStateType disableState);

        ~PlayerAvatarStateMachine() override;

        void OnUpdate     ();
        void OnFixedUpdate();
        void OnDrawGui    ();

        void OnEnable () override;
        void OnDisable() override;
        void OnChangeState(SwordMan::SwordManAvatarStateType type) override;

    protected:
        rxcpp::observable<std::shared_ptr<IPlayerAvatarState>> CurrentState();

    private:
        const StateMap states_;
        rxcpp::subjects::behavior<std::shared_ptr<IPlayerAvatarState>> currentState_;
        const SwordMan::SwordManAvatarStateType initialState_;
        const SwordMan::SwordManAvatarStateType disableState_;
    };
}
