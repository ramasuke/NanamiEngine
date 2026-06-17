#include "PlayerAvatarStateMachine.h"
#include <cassert>

namespace GameCore::PlayerAvatar
{
    PlayerAvatarStateMachine::PlayerAvatarStateMachine(
        StatesFactory factory,
        SwordMan::SwordManAvatarStateType initialState,
        SwordMan::SwordManAvatarStateType disableState)
        : states_      (factory([this](SwordMan::SwordManAvatarStateType type){ OnChangeState(type); }))
        , currentState_(nullptr)
        , initialState_(initialState)
        , disableState_(disableState)
    {
        OnChangeState(initialState_);
    }

    PlayerAvatarStateMachine::~PlayerAvatarStateMachine() = default;

    void PlayerAvatarStateMachine::OnUpdate()
    {
        if (currentState_.get_value())
            currentState_.get_value()->OnUpdate();
    }

    void PlayerAvatarStateMachine::OnFixedUpdate()
    {
        if (currentState_.get_value())
            currentState_.get_value()->OnFixedUpdate();
    }

    void PlayerAvatarStateMachine::OnChangeState(SwordMan::SwordManAvatarStateType type)
    {
        assert(states_.contains(type));

        if (currentState_.get_value())
            currentState_.get_value()->OnExit();

        currentState_.get_subscriber().on_next(states_.at(type));
        currentState_.get_value()->OnEnter();
    }

    void PlayerAvatarStateMachine::OnDrawGui()
    {
        ImGui::Text(("currentState: " + std::string(typeid(*currentState_.get_value()).name())).c_str());

        if (ImGui::TreeNode("States"))
        {
            for (const auto& [stateType, state] : states_)
            {
                const char* typeName = typeid(*state).name();
                if (const bool isCurrent = (currentState_.get_value() == state);
                    ImGui::Selectable(typeName, isCurrent))
                {
                    OnChangeState(stateType);
                }
            }
            ImGui::TreePop();
            ImGui::Spacing();
        }
    }

    void PlayerAvatarStateMachine::OnEnable()
    {
        OnChangeState(initialState_);
    }

    void PlayerAvatarStateMachine::OnDisable()
    {
        OnChangeState(disableState_);
    }

    rxcpp::observable<std::shared_ptr<IPlayerAvatarState>> PlayerAvatarStateMachine::CurrentState()
    {
        return currentState_.get_observable();
    }
}
