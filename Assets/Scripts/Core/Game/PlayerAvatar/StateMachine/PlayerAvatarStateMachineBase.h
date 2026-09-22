#pragma once
#include <cassert>
#include <concepts>
#include <memory>
#include <unordered_map>
#include <functional>
#include "ImGuiHelper.h"
#include "IPlayerAvatarStateMachine.h"
#include "IReadOnlyPlayerAvatarStateMachine.h"
#include "../../../Network/Rpc/Custom_RpcType.h"
#include "../State/IPlayerAvatarState.h"
#include "Packages/R4/R4.h"

namespace GameCore::PlayerAvatar
{
    template<typename T>
    concept Uint8Enum = std::is_enum_v<T> && std::is_same_v<std::underlying_type_t<T>, uint8_t>;

    template<Uint8Enum StateTypeT>
    class PlayerAvatarStateMachineBase : public IPlayerAvatarStateMachine,
                                         public IReadOnlyPlayerAvatarStateMachine<StateTypeT>
    {
    public:
        using StateMap              = std::unordered_map<StateTypeT, std::shared_ptr<IPlayerAvatarState>>;
        using OnChangeStateCallback = std::function<void(StateTypeT)>;
        using StatesFactory         = std::function<StateMap(OnChangeStateCallback)>;

        explicit PlayerAvatarStateMachineBase(
            StatesFactory factory,
            StateTypeT initialState,
            StateTypeT disableState,
            const bool isEnable)
            : states_           (factory([this](StateTypeT type){ OnChangeState(type); }))
            , currentState_     (nullptr)
            , currentStateType_ (initialState)
            , initialState_     (initialState)
            , disableState_     (disableState)
            , isEnable_         (isEnable    )
        {
            OnChangeState(initialState_);
        }

        ~PlayerAvatarStateMachineBase() override = default;

        void OnUpdate() override
        {
            if (!isEnable_)
                return;
            
            if (currentState_.Value())
                currentState_.Value()->OnUpdate();
        }

        void NetworkTick(const Core::Network::NetworkObjectId id, const bool hasStateAuthority) override
        {
            if (!hasStateAuthority)
                return;

            if (id == Core::Network::NetworkObjectId::Invalid())
                return;

            Network::SyncAvatarStateRpc::Send(
                id, Core::Network::DeliveryMode::Unreliable, GetCurrentStateValue());
        }
        
        void OnFixedUpdate() override
        {
            if (!isEnable_)
                return;
            
            if (currentState_.Value())
                currentState_.Value()->OnFixedUpdate();
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
            ImGui::Text(("currentState: " + std::string(typeid(*currentState_.Value()).name())).c_str());

            if (ImGui::TreeNode("States"))
            {
                for (const auto& [stateType, state] : states_)
                {
                    const char* typeName = typeid(*state).name();
                    if (const bool isCurrent = (currentState_.Value() == state);
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
            
            if (currentState_.Value() && isEnable_)
                currentState_.Value()->OnExit();

            currentStateType_ = type;
            currentState_.OnNext(states_.at(type));
            if (isEnable_)
                currentState_.Value()->OnEnter();
        }

        [[nodiscard]] uint8_t GetCurrentStateValue() const
        {
            return static_cast<uint8_t>(currentStateType_);
        }

        [[nodiscard]] StateTypeT GetCurrentStateType() const override
        {
            return currentStateType_;
        }

        virtual void OnEnable()  { OnChangeState(initialState_); }
        virtual void OnDisable() { OnChangeState(disableState_); }

    protected:
        NanamiEngine::R4::Observable<std::shared_ptr<IPlayerAvatarState>> CurrentState()
        {
            return currentState_.AsObservable();
        }

    private:
        bool isEnable_ = false; 
        const StateMap states_;
        // 同じステートへの遷移も通知するので、更新は Value(v) ではなく OnNext(v)
        NanamiEngine::R4::ReactiveProperty<std::shared_ptr<IPlayerAvatarState>> currentState_;
        StateTypeT       currentStateType_;
        const StateTypeT initialState_;
        const StateTypeT disableState_;
    };
}
