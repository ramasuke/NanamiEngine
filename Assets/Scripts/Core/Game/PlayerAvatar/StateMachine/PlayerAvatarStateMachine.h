#pragma once
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <tuple>
#include <cassert>
    
#include "ImGuiHelper.h"
#include "IPlayerAvatarStateMachine.h"
#include "../State/IPlayerAvatarState.h"
#include "../rxcpp/subjects/rx-behavior.hpp"
#include "EventScene/IPlayerAvatarEventSceneStateMachine.h"

namespace GameCore::PlayerAvatar
{
    template <typename T> concept PlayerAvatarState = std::derived_from<T, IPlayerAvatarState>;
    
    template <typename StateT, typename DisableStateT, typename... StateTypes>
    requires PlayerAvatarState<StateT> && (PlayerAvatarState<StateTypes> && ...)
    class PlayerAvatarStateMachine final : public IPlayerAvatarStateMachine,
                                           public IPlayerAvatarEventSceneStateMachine
    {
    public:
        using FirstState = std::tuple_element_t<0, std::tuple<StateTypes...>>;
        
        template <typename StateContextT>
        explicit PlayerAvatarStateMachine(const std::shared_ptr<StateContextT>& context);
        
        void OnUpdate ();
        void OnDrawGui();
        rxcpp::observable<std::shared_ptr<StateT>> CurrentState() { return currentState_.get_observable(); }
        void OnEnable () override;
        void OnDisable() override;
        
    private:
        template <typename StateContextT>
        auto CreateStates(const std::shared_ptr<StateContextT>& context);
        template <typename CreateStateT, typename StateContextT>
        auto CreateState(const std::shared_ptr<StateContextT>& context);
        void OnChangeState(std::type_index type) override;
        
        const std::unordered_map<std::type_index, std::shared_ptr<StateT>> states_;
        rxcpp::subjects::behavior<std::shared_ptr<StateT>> currentState_;
    };

    template <typename StateT, typename DisableStateT, typename... StateTypes>
    requires PlayerAvatarState<StateT> && (PlayerAvatarState<StateTypes> && ...)
    template <typename StateContextT>
    PlayerAvatarStateMachine<StateT, DisableStateT, StateTypes...>::PlayerAvatarStateMachine(const std::shared_ptr<StateContextT>& context)
        : states_      (CreateStates(context))
        , currentState_(nullptr)
    {
        static_assert((std::constructible_from<StateTypes, const std::shared_ptr<StateContextT>&, const std::function<void(std::type_index)>&> && ...),
                      "All states must be constructible from (context, lambda)");
    
        OnChangeState(std::type_index(typeid(FirstState)));
    }

    template <typename StateT, typename DisableStateT, typename... StateTypes>
    requires PlayerAvatarState<StateT> && (PlayerAvatarState<StateTypes> && ...)
    void PlayerAvatarStateMachine<StateT, DisableStateT, StateTypes...>::OnUpdate()
    {
        if (currentState_.get_value())
            currentState_.get_value()->OnUpdate();
    }

    template <typename StateT, typename DisableStateT, typename... StateTypes>
    requires PlayerAvatarState<StateT> && (PlayerAvatarState<StateTypes> && ...)
    void PlayerAvatarStateMachine<StateT, DisableStateT, StateTypes...>::OnChangeState(std::type_index type)
    {
        assert(states_.contains(type) && ("Unknown state type: " + std::string(type.name())).c_str());

        if (currentState_.get_value())
            currentState_.get_value()->OnExit();
        
        currentState_.get_subscriber().on_next(states_.at(type));
        currentState_.get_value()->OnEnter();
    }

    template <typename StateT, typename DisableStateT, typename... StateTypes>
    requires PlayerAvatarState<StateT> && (PlayerAvatarState<StateTypes> && ...)
    void PlayerAvatarStateMachine<StateT, DisableStateT, StateTypes...>::OnDrawGui()
    {
        ImGui::Text(("currentState: " + std::string(typeid(*currentState_.get_value()).name())).c_str());

        if (ImGui::TreeNode("States"))
        {
            for (const auto& [type, state] : states_)
            {
                const char* typeName = type.name();
                if (const bool isCurrent = (currentState_.get_value() == state);
                    ImGui::Selectable(typeName, isCurrent))
                {
                    OnChangeState(type);
                }
            }
            ImGui::TreePop();
            ImGui::Spacing();
        }
    }

    template <typename StateT, typename DisableStateT, typename ... StateTypes>
    requires PlayerAvatarState<StateT> && (PlayerAvatarState<StateTypes> && ...)
    void PlayerAvatarStateMachine<StateT, DisableStateT, StateTypes...>::OnEnable()
    {
        OnChangeState(std::type_index(typeid(FirstState)));
    }

    template <typename StateT, typename DisableStateT, typename ... StateTypes>
    requires PlayerAvatarState<StateT> && (PlayerAvatarState<StateTypes> && ...)
    void PlayerAvatarStateMachine<StateT, DisableStateT, StateTypes...>::OnDisable()
    {
        OnChangeState(typeid(DisableStateT));
    }

    template <typename StateT, typename DisableStateT, typename... StateTypes>
    requires PlayerAvatarState<StateT> && (PlayerAvatarState<StateTypes> && ...)
    template <typename StateContextT>
    auto PlayerAvatarStateMachine<StateT, DisableStateT, StateTypes...>::CreateStates(
    const std::shared_ptr<StateContextT>& context)
    {
        std::unordered_map<std::type_index, std::shared_ptr<StateT>> result;
        result.emplace(CreateState<DisableStateT>(context));
        (result.emplace(CreateState<StateTypes>(context)), ...);

        return result;
    }


    template <typename StateT, typename DisableStateT, typename... StateTypes>
    requires PlayerAvatarState<StateT> && (PlayerAvatarState<StateTypes> && ...)
    template <typename CreateStateT, typename StateContextT>
    auto PlayerAvatarStateMachine<StateT, DisableStateT, StateTypes...>::CreateState(
        const std::shared_ptr<StateContextT>& context)
    {
        static_assert(
            std::constructible_from<CreateStateT, const std::shared_ptr<StateContextT>&, const std::function<void(std::type_index)>&>
            ,"State must be constructible from (context, changeStateCallback)"
        );
        
        return std::pair<std::type_index, std::shared_ptr<StateT>>{
            std::type_index(typeid(CreateStateT)),
            std::make_shared<CreateStateT>(
                context,
                [this](const std::type_index type)
                {
                    OnChangeState(type);
                })
        };
    }
}
