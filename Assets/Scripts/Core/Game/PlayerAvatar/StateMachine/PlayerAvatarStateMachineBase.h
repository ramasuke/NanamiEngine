#pragma once
#include <cassert>
#include <concepts>
#include <memory>
#include <unordered_map>
#include <functional>
#include "ImGuiHelper.h"
#include "IPlayerAvatarStateMachine.h"
#include "../../../../GamePlay/Network/Game_CustomNetworkRunner.h"
#include "../State/IPlayerAvatarState.h"
#include "../rxcpp/subjects/rx-behavior.hpp"

namespace GameCore::PlayerAvatar
{
    template<typename T>
    concept Uint8Enum = std::is_enum_v<T> && std::is_same_v<std::underlying_type_t<T>, uint8_t>;

    template<Uint8Enum StateTypeT>
    class PlayerAvatarStateMachineBase : public IPlayerAvatarStateMachine
    {
    public:
        using StateMap              = std::unordered_map<StateTypeT, std::shared_ptr<IPlayerAvatarState>>;
        using OnChangeStateCallback = std::function<void(StateTypeT)>;
        using StatesFactory         = std::function<StateMap(OnChangeStateCallback)>;

        explicit PlayerAvatarStateMachineBase(
            StatesFactory factory,
            StateTypeT initialState,
            StateTypeT disableState)
            : states_           (factory([this](StateTypeT type){ OnChangeState(type); }))
            , currentState_     (nullptr)
            , currentStateType_ (initialState)
            , initialState_     (initialState)
            , disableState_     (disableState)
        {
            OnChangeState(initialState_);
        }

        ~PlayerAvatarStateMachineBase() override = default;

        void OnUpdate() override
        {
            if (currentState_.get_value())
                currentState_.get_value()->OnUpdate();
        }

        void NetworkTick(const Core::Network::NetworkObjectId id, const bool hasStateAuthority) override
        {
            if (!hasStateAuthority)
                return;
            
            if (id == Core::Network::NetworkObjectId::Invalid())
                return;

            GamePlay::Network::CustomNetworkRunner::Instance()
                .CustomDispatcher()
                .SyncAvatarState()
                .DispatchSendPacket(id, GetCurrentStateValue());
        }
        
        void OnFixedUpdate() override
        {
            if (currentState_.get_value())
                currentState_.get_value()->OnFixedUpdate();
        }

        void ApplySyncState(const uint8_t stateValue)
        {
            const auto type = static_cast<StateTypeT>(stateValue);
            if (!states_.contains(type))
                return;
            
            OnChangeState(type);
        }
        
        void OnDrawGui()
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

        virtual void OnChangeState(StateTypeT type)
        {
            assert(states_.contains(type));

            if (currentState_.get_value())
                currentState_.get_value()->OnExit();

            currentStateType_ = type;
            currentState_.get_subscriber().on_next(states_.at(type));
            currentState_.get_value()->OnEnter();
        }

        [[nodiscard]] uint8_t GetCurrentStateValue() const
        {
            return static_cast<uint8_t>(currentStateType_);
        }

        virtual void OnEnable()  { OnChangeState(initialState_); }
        virtual void OnDisable() { OnChangeState(disableState_); }

    protected:
        rxcpp::observable<std::shared_ptr<IPlayerAvatarState>> CurrentState()
        {
            return currentState_.get_observable();
        }

    private:
        const StateMap states_;
        rxcpp::subjects::behavior<std::shared_ptr<IPlayerAvatarState>> currentState_;
        StateTypeT       currentStateType_;
        const StateTypeT initialState_;
        const StateTypeT disableState_;
    };
}
